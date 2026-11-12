#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <clocale>
#include <cwchar>
#include <fstream>
#include <stdexcept>
#include "core.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

namespace wrt {

class unique_handle {
public:
    explicit unique_handle(HANDLE value = nullptr) : value_(value) {}
    ~unique_handle() { reset(); }
    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;
    HANDLE get() const { return value_; }
    explicit operator bool() const { return value_ && value_ != INVALID_HANDLE_VALUE; }
    void reset(HANDLE value = nullptr) {
        if (*this) CloseHandle(value_);
        value_ = value;
    }
private:
    HANDLE value_;
};

inline std::filesystem::path module_path() {
    std::wstring buffer(32768, L'\0');
    const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!size || size >= buffer.size()) throw std::runtime_error("Cannot determine executable path");
    buffer.resize(size);
    return buffer;
}

inline std::filesystem::path install_directory() {
    return module_path().parent_path().parent_path();
}

inline std::filesystem::path windows_directory() {
    wchar_t buffer[MAX_PATH] = {};
    const UINT size = GetWindowsDirectoryW(buffer, MAX_PATH);
    if (!size || size >= MAX_PATH) throw std::runtime_error("Cannot determine Windows directory");
    return buffer;
}

inline std::filesystem::path system_program(const wchar_t* name) {
    wchar_t buffer[MAX_PATH] = {};
    const UINT size = GetSystemDirectoryW(buffer, MAX_PATH);
    if (!size || size >= MAX_PATH) throw std::runtime_error("Cannot determine system directory");
    return std::filesystem::path(buffer) / name;
}

inline std::filesystem::path program_path(tool program) {
    switch (program) {
    case tool::defrag: return system_program(L"defrag.exe");
    case tool::sfc: return system_program(L"sfc.exe");
    case tool::chkdsk: return system_program(L"chkdsk.exe");
    case tool::dism: return system_program(L"dism.exe");
    case tool::cleanup: return system_program(L"cleanmgr.exe");
    case tool::defender: {
        PWSTR folder = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &folder)))
            throw std::runtime_error("Cannot determine Defender directory");
        const auto result = std::filesystem::path(folder) / L"Windows Defender" / L"MpCmdRun.exe";
        CoTaskMemFree(folder);
        return result;
    }
    }
    throw std::runtime_error("Unknown repair program");
}

inline std::wstring command_line(const std::filesystem::path& executable, const std::wstring& arguments) {
    return quote_argument(executable.wstring()) + (arguments.empty() ? L"" : L" " + arguments);
}

inline process_result run_attached(const std::filesystem::path& executable,
    const std::wstring& arguments, const std::filesystem::path& directory = {},
    bool minimized = false) {
    process_result result;
    if (!executable.is_absolute()) { result.error = ERROR_BAD_PATHNAME; return result; }
    auto command = command_line(executable, arguments);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    if (minimized) {
        startup.dwFlags |= STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_SHOWMINNOACTIVE;
    }
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NEW_PROCESS_GROUP,
        nullptr, directory.empty() ? nullptr : directory.c_str(), &startup, &process)) {
        result.error = GetLastError();
        return result;
    }
    unique_handle process_handle(process.hProcess), thread_handle(process.hThread);
    result.started = true;
    DWORD exit_code = 0;
    if (WaitForSingleObject(process_handle.get(), INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(process_handle.get(), &exit_code)) result.error = GetLastError();
    else { result.completed = true; result.exit_code = exit_code; }
    return result;
}

inline process_result run_process(const std::filesystem::path& executable,
    const std::wstring& arguments, const std::function<void(std::string_view)>& output = {},
    std::string_view input = {}) {
    process_result result;
    if (!executable.is_absolute() || input.size() > 1024) {
        result.error = ERROR_INVALID_PARAMETER;
        return result;
    }
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE read = nullptr, write = nullptr;
    if (!CreatePipe(&read, &write, &security, 0)) { result.error = GetLastError(); return result; }
    unique_handle stdout_read(read), stdout_write(write);
    if (!SetHandleInformation(stdout_read.get(), HANDLE_FLAG_INHERIT, 0)) {
        result.error = GetLastError(); return result;
    }
    if (!CreatePipe(&read, &write, &security, 0)) { result.error = GetLastError(); return result; }
    unique_handle stdin_read(read), stdin_write(write);
    if (!SetHandleInformation(stdin_write.get(), HANDLE_FLAG_INHERIT, 0)) {
        result.error = GetLastError(); return result;
    }

    SIZE_T attribute_size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_size);
    std::vector<unsigned char> storage(attribute_size);
    auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attribute_size)) {
        result.error = GetLastError(); return result;
    }
    struct attribute_guard {
        LPPROC_THREAD_ATTRIBUTE_LIST list;
        ~attribute_guard() { DeleteProcThreadAttributeList(list); }
    } guard{attributes};
    HANDLE inherited[] = {stdin_read.get(), stdout_write.get()};
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        inherited, sizeof(inherited), nullptr, nullptr)) {
        result.error = GetLastError(); return result;
    }
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = stdin_read.get();
    startup.StartupInfo.hStdOutput = stdout_write.get();
    startup.StartupInfo.hStdError = stdout_write.get();
    startup.lpAttributeList = attributes;
    auto command = command_line(executable, arguments);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr,
        &startup.StartupInfo, &process)) {
        result.error = GetLastError(); return result;
    }
    unique_handle process_handle(process.hProcess), thread_handle(process.hThread);
    result.started = true;
    stdout_write.reset();
    stdin_read.reset();
    if (!input.empty()) {
        DWORD written = 0;
        if (!WriteFile(stdin_write.get(), input.data(), static_cast<DWORD>(input.size()), &written, nullptr))
            result.error = GetLastError();
        else if (written != input.size()) result.error = ERROR_WRITE_FAULT;
    }
    stdin_write.reset(); // Deliver EOF; never leave a child blocked waiting for input.
    char buffer[4096];
    DWORD count = 0;
    bool output_failed = false;
    while (true) {
        if (!ReadFile(stdout_read.get(), buffer, sizeof(buffer), &count, nullptr)) {
            const DWORD error = GetLastError();
            if (error != ERROR_BROKEN_PIPE) result.error = error;
            break;
        }
        if (!count) break;
        if (output && !output_failed) {
            try { output(std::string_view(buffer, count)); }
            catch (...) { result.error = ERROR_WRITE_FAULT; output_failed = true; }
        }
    }
    stdout_read.reset();
    DWORD exit_code = 0;
    if (WaitForSingleObject(process_handle.get(), INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(process_handle.get(), &exit_code)) result.error = GetLastError();
    else { result.completed = true; result.exit_code = exit_code; }
    return result;
}

inline HMODULE load_language_resource(const std::filesystem::path& root, std::wstring_view locale) {
    const auto language = language_name(locale);
    if (language.empty() || !root.is_absolute()) return nullptr;
    const auto file = root / L"locale" / (language + L".dll");
    return LoadLibraryExW(file.c_str(), nullptr,
        LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
}

inline int load_language(void (*load_strings)(HINSTANCE), UINT codepage_id) {
    wchar_t locale[LOCALE_NAME_MAX_LENGTH] = {};
    HMODULE resource = nullptr;
    if (GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH))
        resource = load_language_resource(install_directory(), locale);
    SetConsoleOutputCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF8");
    if (!resource) { load_strings(GetModuleHandleW(nullptr)); return 1; }
    (void)codepage_id; // Strings are UTF-16 resources; SFC output is decoded to UTF-8.
    load_strings(resource);
    FreeLibrary(resource);
    return 0;
}

inline void load_string(HINSTANCE source, UINT id, wchar_t* buffer, int capacity) {
    if (!LoadStringW(source, id, buffer, capacity))
        LoadStringW(GetModuleHandleW(nullptr), id, buffer, capacity);
}

// Text as UTF-8 bytes, for log files and any other byte-oriented sink.
inline std::string utf8(std::wstring_view text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        result.data(), size, nullptr, nullptr)) return {};
    return result;
}

inline std::wstring timestamp() {
    SYSTEMTIME time{};
    GetLocalTime(&time);
    wchar_t buffer[24] = {};
    if (std::swprintf(buffer, 24, L"%04u-%02u-%02u %02u:%02u:%02u",
        static_cast<unsigned>(time.wYear), static_cast<unsigned>(time.wMonth),
        static_cast<unsigned>(time.wDay), static_cast<unsigned>(time.wHour),
        static_cast<unsigned>(time.wMinute), static_cast<unsigned>(time.wSecond)) < 0)
        return {};
    return buffer;
}

// Append-only session log. Logging is strictly best-effort: a directory or
// file that cannot be created or opened disables the logger instead of
// throwing, so logging can never interfere with a repair.
class step_logger {
public:
    step_logger() = default;
    explicit step_logger(const std::filesystem::path& file) {
        std::error_code error;
        std::filesystem::create_directories(file.parent_path(), error);
        stream_.open(file, std::ios::out | std::ios::app | std::ios::binary);
    }
    // std::ofstream's bool conversion is unreliable across runtimes (MSVCRT
    // reports a default-constructed stream as good), so check is_open().
    bool active() const { return stream_.is_open(); }
    // One timestamped line of text.
    void line(std::wstring_view text) {
        if (!stream_.is_open()) return;
        std::string bytes = "[";
        bytes += utf8(timestamp());
        bytes += "] ";
        bytes += utf8(text);
        bytes += '\n';
        write(bytes);
    }
    // Verbatim diagnostic output (already UTF-8); a missing trailing newline
    // is added so consecutive dumps stay on separate lines.
    void dump(std::string_view text) {
        if (!stream_.is_open() || text.empty()) return;
        std::string bytes(text);
        if (bytes.back() != '\n') bytes += '\n';
        write(bytes);
    }
private:
    void write(const std::string& bytes) {
        if (!stream_.is_open()) return;
        stream_ << bytes;
        stream_.flush();
    }
    std::ofstream stream_;
};

// %ProgramData%\WRT\logs, created on demand. Empty when the profile folders
// are unavailable; never throws — logging is optional.
inline std::optional<std::filesystem::path> program_data_logs_dir() {
    PWSTR folder = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &folder))) return std::nullopt;
    std::optional<std::filesystem::path> result;
    try {
        std::filesystem::path directory = std::filesystem::path(folder) / L"WRT" / L"logs";
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (!error) result = directory;
    } catch (...) {
    }
    CoTaskMemFree(folder);
    return result;
}

} // namespace wrt
