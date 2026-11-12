#include "windows_update.h"
#include "windows_issues.h"
#include <fstream>
#include <iostream>
#include <iterator>

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition); } while (false)

int main() {
    try {
        int argc = 0;
        auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        CHECK(argv && argc == 4);
        const std::filesystem::path fixture(argv[1]), resource_root(argv[2]), package_root(argv[3]);
        LocalFree(argv);

        CHECK(wrt::system_program(L"sfc.exe").is_absolute());
        CHECK(wrt::program_path(wrt::tool::defender).is_absolute());
        CHECK(wrt::module_path().is_absolute());
        CHECK(!wrt::run_process(L"fixture.exe", L"").started);
        CHECK(!wrt::run_attached(L"fixture.exe", L"").started);

        auto result = wrt::run_process(fixture.parent_path() / L"does-not-exist.exe", L"");
        CHECK(!result.started && !result.completed && result.error != 0);
        result = wrt::run_attached(fixture, L"--expect-minimized", {}, true);
        CHECK(result.completed && result.error == 0 && result.exit_code == 0);

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
        CHECK(wrt::issue_text(ISSUE_MODE_SIMPLE).find(L"several minutes") != std::wstring::npos);
        module = wrt::load_language_resource(package_root, L"de-DE");
        CHECK(module != nullptr);
        wchar_t translated[2048] = {};
        wrt::load_string(module, ISSUE_MODE_SIMPLE, translated, 2048);
        CHECK(std::wstring(translated).find(L"einige Minuten") != std::wstring::npos);
        wrt::load_string(module, ISSUE_HELP, translated, 2048);
        CHECK(std::wstring(translated).find(L"https://learn.microsoft.com/") != std::wstring::npos);
        FreeLibrary(module);

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

        // Missing shortcuts are restored byte-for-byte; existing user files
        // are never replaced, even when another shortcut is missing.
        CHECK(wrt::ensure_batch_shortcuts(L"relative") == ERROR_BAD_PATHNAME);
        CHECK(wrt::ensure_batch_shortcuts(temporary) == ERROR_SUCCESS);
        const auto scripts = temporary / L"Batch scripts";
        unsigned mode = 0;
        for (const auto name : {L"Simple repair.bat", L"Default repair.bat", L"Extended repair.bat"}) {
            std::ifstream script(scripts / name, std::ios::binary);
            const std::string bytes((std::istreambuf_iterator<char>(script)), std::istreambuf_iterator<char>());
            CHECK(bytes == wrt::batch_shortcut(++mode));
        }
        { std::ofstream custom(scripts / L"Simple repair.bat"); custom << "custom script"; }
        CHECK(std::filesystem::remove(scripts / L"Default repair.bat"));
        CHECK(wrt::ensure_batch_shortcuts(temporary) == ERROR_SUCCESS);
        {
            std::ifstream custom(scripts / L"Simple repair.bat");
            std::string bytes((std::istreambuf_iterator<char>(custom)), std::istreambuf_iterator<char>());
            CHECK(bytes == "custom script");
        }
        CHECK(std::filesystem::remove(scripts / L"Extended repair.bat"));
        CHECK(std::filesystem::create_directory(scripts / L"Extended repair.bat"));
        CHECK(wrt::ensure_batch_shortcuts(temporary) != ERROR_SUCCESS);

        DWORD input_mode = 0;
        const auto input = GetStdHandle(STD_INPUT_HANDLE);
        {
            wrt::repair_console console;
            DWORD mode = 0;
            if (!GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode)) {
                CHECK(std::wstring(console.color(L"\x1b[91m")).empty());
                CHECK(console.columns() == 0);
            }
            if (!GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &mode))
                CHECK(std::wstring(console.color(L"\x1b[91m", true)).empty());
        }
        if (GetConsoleMode(input, &input_mode)) {
            {
                wrt::repair_console console;
                DWORD during = 0;
                CHECK(GetConsoleMode(input, &during));
                CHECK((during & ENABLE_QUICK_EDIT_MODE) == 0);
            }
            DWORD restored = 0;
            CHECK(GetConsoleMode(input, &restored) && restored == input_mode);
        }

        // The step logger writes timestamped UTF-8 lines and verbatim dumps,
        // and disables itself instead of throwing when the file is unusable.
        {
            wrt::step_logger session(temporary / L"logs" / L"session.log");
            CHECK(session.active());
            session.line(L"Reparatur l\u00e4uft \u2014 Schritt 1");
            session.dump("diagnostic tail \xF0\x9F\x98\x80");
            session.dump("no trailing newline");
        }
        std::ifstream logged(temporary / L"logs" / L"session.log", std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(logged)), std::istreambuf_iterator<char>());
        CHECK(content.find("[20") == 0);
        CHECK(content.find("] Reparatur l\xc3\xa4uft \xe2\x80\x94 Schritt 1\n") != std::string::npos);
        CHECK(content.find("diagnostic tail \xF0\x9F\x98\x80\n") != std::string::npos);
        CHECK(content.find("no trailing newline\n") != std::string::npos);
        { std::ofstream blocker(temporary / L"blocker", std::ios::binary | std::ios::trunc); blocker << 'x'; }
        wrt::step_logger blocked(temporary / L"blocker" / L"child.log");
        CHECK(!blocked.active());
        wrt::step_logger silent;
        CHECK(!silent.active());
        silent.line(L"ignored");
        silent.dump("ignored");
        const auto logs_directory = wrt::program_data_logs_dir(); // Must not throw; result is environment-dependent.
        (void)logs_directory;

        std::cout << "PASS Windows process, quoting, resource loading, CWD isolation and unsigned-file rejection\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
