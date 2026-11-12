#include "windows_update.h"
#include <iostream>

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition); } while (false)

int main() {
    try {
        int argc = 0;
        auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        CHECK(argv && argc == 3);
        const std::filesystem::path fixture(argv[1]), resource_root(argv[2]);
        LocalFree(argv);

        CHECK(wrt::system_program(L"sfc.exe").is_absolute());
        CHECK(wrt::program_path(wrt::tool::defender).is_absolute());
        CHECK(wrt::module_path().is_absolute());
        CHECK(!wrt::run_process(L"fixture.exe", L"").started);
        CHECK(!wrt::run_attached(L"fixture.exe", L"").started);

        auto result = wrt::run_process(fixture.parent_path() / L"does-not-exist.exe", L"");
        CHECK(!result.started && !result.completed && result.error != 0);

        std::string output;
        result = wrt::run_process(fixture, L"--emit", [&](std::string_view bytes) { output.append(bytes); });
        CHECK(result.started && result.completed && result.error == 0 && result.exit_code == 42);
        CHECK(output.size() == 518 && output[256] == '\0' && output.substr(512) == "stderr");
        std::size_t total = 0;
        result = wrt::run_process(fixture, L"--large", [&](std::string_view bytes) { total += bytes.size(); });
        CHECK(result.completed && result.error == 0 && result.exit_code == 0 && total == 1048576);
        // Silent mode must still drain the pipe, even when the child fills it.
        result = wrt::run_process(fixture, L"--large");
        CHECK(result.completed && result.error == 0 && result.exit_code == 0);
        result = wrt::run_process(fixture, L"--large", [](std::string_view) {
            throw std::runtime_error("simulated output failure");
        });
        CHECK(result.started && result.completed && result.error == ERROR_WRITE_FAULT && result.exit_code == 0);
        output.clear();
        result = wrt::run_process(fixture, L"--stdin", [&](std::string_view bytes) { output.append(bytes); }, "hello\r\n");
        CHECK(result.completed && result.error == 0 && result.exit_code == 0 && output == "hello\r\n");
        result = wrt::run_process(fixture, L"--stdin");
        CHECK(result.completed && result.error == 0 && result.exit_code == 0); // EOF without input.

        for (const auto& argument : {std::wstring(L""), std::wstring(L"a b\\"), std::wstring(L"a\\\"b&c"),
            std::wstring(L"\u00e4\u4e2d")}) {
            output.clear();
            result = wrt::run_process(fixture, L"--argument " + wrt::quote_argument(argument),
                [&](std::string_view bytes) { output.append(bytes); });
            CHECK(result.completed && result.error == 0 && result.exit_code == 0);
            const int length = MultiByteToWideChar(CP_UTF8, 0, output.data(), static_cast<int>(output.size()), nullptr, 0);
            std::wstring decoded(static_cast<std::size_t>(length), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, output.data(), static_cast<int>(output.size()), decoded.data(), length);
            CHECK(decoded == argument);
        }

        SetEnvironmentVariableW(L"WRT_TEST_DLL_EXECUTED", nullptr);
        HMODULE module = wrt::load_language_resource(resource_root, L"en-US");
        CHECK(module != nullptr);
        wchar_t text[64] = {};
        CHECK(LoadStringW(module, 9001, text, 64) != 0 && std::wstring(text) == L"fixture localization");
        wrt::load_string(module, 9002, text, 64);
        CHECK(std::wstring(text) == L"built-in English fallback");
        CHECK(GetEnvironmentVariableW(L"WRT_TEST_DLL_EXECUTED", text, 64) == 0);
        FreeLibrary(module);
        CHECK(wrt::load_language_resource(resource_root, L"../en") == nullptr);
        CHECK(wrt::load_language_resource(L"relative", L"en-US") == nullptr);

        const auto current = std::filesystem::current_path();
        // This temporary directory belongs only to this test run.
        GUID id{};
        wchar_t name[40] = {};
        CHECK(SUCCEEDED(CoCreateGuid(&id)) && StringFromGUID2(id, name, 40));
        const auto temporary = std::filesystem::temp_directory_path() / (std::wstring(L"wrt-test-") + name);
        CHECK(std::filesystem::create_directory(temporary));
        struct cleanup {
            std::filesystem::path current, directory;
            ~cleanup() {
                std::error_code ignored;
                std::filesystem::current_path(current, ignored);
                std::filesystem::remove_all(directory, ignored);
            }
        } guard{current, temporary};
        std::filesystem::create_directories(temporary / L"locale");
        std::filesystem::copy_file(resource_root / L"locale" / L"en.dll", temporary / L"locale" / L"de.dll");
        std::filesystem::current_path(temporary);
        CHECK(wrt::load_language_resource(resource_root, L"de-DE") == nullptr);
        CHECK(GetEnvironmentVariableW(L"WRT_TEST_DLL_EXECUTED", text, 64) == 0);
        // The executable path, not the working directory, selects the child.
        result = wrt::run_process(fixture, L"--stdin");
        CHECK(result.completed && result.error == 0 && result.exit_code == 0);
        CHECK(wrt::trusted_signer(temporary / L"missing.msi").empty());
        CHECK(wrt::trusted_signer(wrt::module_path()).empty()); // Test binary is unsigned.
        std::cout << "PASS Windows process, quoting, resource loading, CWD isolation and unsigned-file rejection\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
