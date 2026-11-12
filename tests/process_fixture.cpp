#include "windows_support.h"
#include <iostream>

namespace {
bool write_output(HANDLE stream, std::string_view bytes) {
    DWORD written = 0;
    return WriteFile(stream, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)
        && written == bytes.size();
}
} // namespace

int main() {
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 100;
    std::vector<std::wstring> arguments;
    for (int i = 0; i < argc; ++i) arguments.emplace_back(argv[i]);
    LocalFree(argv);
    if (arguments.size() < 2) return 100;
    if (arguments[1] == L"--expect-minimized") {
        STARTUPINFOW startup{};
        GetStartupInfoW(&startup);
        return (startup.dwFlags & STARTF_USESHOWWINDOW) && startup.wShowWindow == SW_SHOWMINNOACTIVE ? 0 : 100;
    }
    if (arguments[1] == L"--emit") {
        std::string bytes(512, 'A');
        bytes[256] = '\0';
        if (!write_output(GetStdHandle(STD_OUTPUT_HANDLE), bytes) ||
            !write_output(GetStdHandle(STD_ERROR_HANDLE), "stderr")) return 100;
        return 42;
    }
    if (arguments[1] == L"--large") {
        const std::string bytes(4096, 'x');
        for (int i = 0; i < 256; ++i)
            if (!write_output(GetStdHandle(STD_OUTPUT_HANDLE), bytes)) return 100;
        return 0;
    }
    if (arguments[1] == L"--stdin") {
        char buffer[512];
        DWORD count = 0;
        while (ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, sizeof(buffer), &count, nullptr) && count)
            if (!write_output(GetStdHandle(STD_OUTPUT_HANDLE), {buffer, count})) return 100;
        return 0;
    }
    if (arguments[1] == L"--argument" && arguments.size() == 3) {
        const auto& argument = arguments[2];
        const int count = WideCharToMultiByte(CP_UTF8, 0, argument.data(), static_cast<int>(argument.size()), nullptr, 0, nullptr, nullptr);
        std::string output(static_cast<std::size_t>(count), '\0');
        WideCharToMultiByte(CP_UTF8, 0, argument.data(), static_cast<int>(argument.size()), output.data(), count, nullptr, nullptr);
        return write_output(GetStdHandle(STD_OUTPUT_HANDLE), output) ? 0 : 100;
    }
    return 100;
}
