#pragma once

#include "windows_support.h"
#include "issue_support.h"
#include "issue_resources.h"
#include <cstring>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

namespace wrt {

inline std::wstring issue_text(UINT id) {
    wchar_t text[2048] = {};
    wchar_t locale[LOCALE_NAME_MAX_LENGTH] = {};
    HMODULE resource = nullptr;
    if (GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH))
        resource = load_language_resource(install_directory(), locale);
    load_string(resource ? resource : GetModuleHandleW(nullptr), id, text, 2048);
    if (resource) FreeLibrary(resource);
    return text;
}

inline bool native_platform_supported() {
    const auto ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll || GetProcAddress(ntdll, "wine_get_version")) return false;
    const auto address = GetProcAddress(ntdll, "RtlGetVersion");
    using version_query = LONG (WINAPI*)(OSVERSIONINFOW*);
    version_query query = nullptr;
    static_assert(sizeof(query) == sizeof(address));
    std::memcpy(&query, &address, sizeof(query));
    OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    return query && query(&version) == 0 && supported_platform(version.dwMajorVersion,
        sizeof(void*) == 8, info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64, false);
}

// Keep the existing named-object protocol: do not leave a second console
// waiting indefinitely (or kill another instance's active repair).
inline bool wait_for_instance(unique_handle& mutex, const wchar_t* name = L"Local\\WRT") {
    for (;;) {
        mutex.reset(CreateMutexW(nullptr, FALSE, name));
        const DWORD error = GetLastError();
        if (!mutex) throw std::runtime_error("Cannot create application lock: " + std::to_string(error));
        if (error != ERROR_ALREADY_EXISTS) return true;
        mutex.reset();
        if (MessageBoxW(GetConsoleWindow(), issue_text(ISSUE_INSTANCE_BUSY).c_str(), L"Windows Repair Tool",
            MB_RETRYCANCEL | MB_ICONINFORMATION | MB_SETFOREGROUND) != IDRETRY) return false;
    }
}

inline bool pending_reboot() {
    const auto file = windows_directory() / L"WinSxS" / L"pending.xml";
    if (GetFileAttributesW(file.c_str()) != INVALID_FILE_ATTRIBUTES) return true;
    for (const auto key : {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\RebootPending",
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\RebootRequired"}) {
        HKEY opened = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &opened) == ERROR_SUCCESS) {
            RegCloseKey(opened);
            return true;
        }
    }
    return false;
}

inline DWORD ensure_batch_shortcuts(const std::filesystem::path& root) {
    if (!root.is_absolute()) return ERROR_BAD_PATHNAME;
    const auto directory = root / L"Batch scripts";
    if (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
        return GetLastError();
    const auto attributes = GetFileAttributesW(directory.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return GetLastError();
    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        return ERROR_ACCESS_DENIED;
    const wchar_t* names[] = {L"Simple repair.bat", L"Default repair.bat", L"Extended repair.bat"};
    for (unsigned i = 0; i < 3; ++i) {
        const auto file = directory / names[i];
        unique_handle handle(CreateFileW(file.c_str(), GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!handle) {
            const auto error = GetLastError();
            if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) return error;
            const auto existing = GetFileAttributesW(file.c_str());
            if (existing == INVALID_FILE_ATTRIBUTES) return GetLastError();
            if (existing & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) return ERROR_ACCESS_DENIED;
            continue; // Never overwrite an existing script, including customized ones.
        }
        const auto bytes = batch_shortcut(i + 1);
        DWORD written = 0;
        if (!WriteFile(handle.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) ||
            written != bytes.size() || !FlushFileBuffers(handle.get())) {
            const DWORD error = GetLastError();
            // Delete only the incomplete file we just created, by its open handle.
            FILE_DISPOSITION_INFO remove{TRUE};
            SetFileInformationByHandle(handle.get(), FileDispositionInfo, &remove, sizeof(remove));
            return error ? error : ERROR_WRITE_FAULT;
        }
    }
    return ERROR_SUCCESS;
}

inline void restore_console(bool center = false) {
    const auto window = GetConsoleWindow();
    if (!window || !IsWindowVisible(window)) return; // Do not move Windows Terminal's hidden console window.
    if (IsIconic(window)) ShowWindow(window, SW_RESTORE);
    if (!center) return;
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    RECT rect{};
    if (!GetWindowRect(window, &rect) ||
        !GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor)) return;
    const LONG width = std::min(rect.right - rect.left, monitor.rcWork.right - monitor.rcWork.left);
    const LONG height = std::min(rect.bottom - rect.top, monitor.rcWork.bottom - monitor.rcWork.top);
    SetWindowPos(window, nullptr, monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2,
        monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2,
        width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

class repair_console {
public:
    repair_console() : input_(GetStdHandle(STD_INPUT_HANDLE)), output_(GetStdHandle(STD_OUTPUT_HANDLE)) {
        input_saved_ = GetConsoleMode(input_, &input_mode_) != FALSE;
        if (input_saved_)
            SetConsoleMode(input_, (input_mode_ | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
        output_saved_ = GetConsoleMode(output_, &output_mode_) != FALSE;
        colors_ = output_saved_ && SetConsoleMode(output_, output_mode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    ~repair_console() {
        if (input_saved_) SetConsoleMode(input_, input_mode_);
        if (output_saved_) SetConsoleMode(output_, output_mode_);
    }
    repair_console(const repair_console&) = delete;
    repair_console& operator=(const repair_console&) = delete;
    int columns() const {
        CONSOLE_SCREEN_BUFFER_INFO info{};
        return GetConsoleScreenBufferInfo(output_, &info) ? info.srWindow.Right - info.srWindow.Left + 1 : 0;
    }
    const wchar_t* color(const wchar_t* sequence, bool error_stream = false) const {
        DWORD mode = 0;
        const auto sink = error_stream ? GetStdHandle(STD_ERROR_HANDLE) : output_;
        return colors_ && GetConsoleMode(sink, &mode) && (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
            ? sequence : L"";
    }
    void redraw(std::wostream& stream, std::wstring_view text, std::size_t& width) const {
        const int count = columns();
        if (count <= 1) return;
        width = std::min(width, static_cast<std::size_t>(count - 1));
        redraw_line(stream, fit_status_line(text, count), width);
        stream.flush();
    }
private:
    HANDLE input_, output_;
    DWORD input_mode_ = 0, output_mode_ = 0;
    bool input_saved_ = false, output_saved_ = false, colors_ = false;
};

} // namespace wrt
