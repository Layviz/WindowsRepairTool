#include "issue_support.h"
#include "update.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition); } while (false)

int main(int argc, char** argv) {
    try {
        using namespace wrt;
        using namespace std::chrono;
        CHECK(supported_platform(10, true, true, false));
        CHECK(supported_platform(11, true, true, false));
        CHECK(!supported_platform(6, true, true, false));
        CHECK(!supported_platform(10, false, true, false));
        CHECK(!supported_platform(10, true, false, false));
        CHECK(!supported_platform(10, true, true, true));

        CHECK(elapsed_seconds(milliseconds(0)) == L"0.0");
        CHECK(elapsed_seconds(milliseconds(999)) == L"0.9");
        CHECK(elapsed_seconds(milliseconds(1000)) == L"1.0");
        CHECK(elapsed_seconds(milliseconds(1234)) == L"1.2");
        CHECK(elapsed_seconds(milliseconds(60000)) == L"60.0");
        CHECK(elapsed_seconds(milliseconds(86400000)) == L"86400.0");
        CHECK(elapsed_seconds(milliseconds(-1)) == L"0.0");

        for (const std::string text : {"Verification 62.3% complete", "[== 62,35% ==]", " 62%"}) {
            for (std::size_t split = 0; split <= text.size(); ++split) {
                progress_reader reader;
                const auto first = reader.feed(std::string_view(text).substr(0, split));
                const auto second = reader.feed(std::string_view(text).substr(split));
                CHECK((second ? second : first) == 62u);
            }
        }
        progress_reader reader;
        CHECK(!reader.feed("6"));
        CHECK(!reader.feed("2."));
        CHECK(reader.feed("3%") == 62u);
        CHECK(!reader.feed(" done"));
        CHECK(reader.feed(" 10% 11%") == 11u);
        for (const std::string text : {"x3%", "101%", "62.356%", "1,234%", "999999999999%"}) {
            progress_reader invalid;
            CHECK(!invalid.feed(text));
        }
        CHECK(!reader.feed(std::string(1000, '9') + "%"));
        CHECK(reader.feed(" 42%") == 42u);

        for (const std::string text : {
            "There is a system repair pending which requires reboot to complete.",
            "The changes will take effect after the next reboot.",
            "Es steht eine Systemreparatur aus, deren Abschluss einen Neustart erfordert.",
            "Bitte beachten: NEUSTART ERFORDERLICH."}) {
            for (std::size_t split = 0; split <= text.size(); ++split) {
                reboot_reader reboot;
                reboot.feed(std::string_view(text).substr(0, split));
                reboot.feed(std::string_view(text).substr(split));
                CHECK(reboot.required());
            }
        }
        reboot_reader reboot;
        CHECK(!reboot.feed("No restart is required. Kein Neustart notwendig. 100% complete."));
        CHECK(reboot.feed(std::string(10000, 'x') + " requires reboot"));
        CHECK(reboot.feed(" done"));

        CHECK(fit_status_line(L"abcdef", 0).empty());
        CHECK(fit_status_line(L"abcdef", 1).empty());
        CHECK(fit_status_line(L"abcdef", 2) == L"a");
        CHECK(fit_status_line(L"abcdef", 6) == L"ab...");
        CHECK(fit_status_line(L"abcdef", 7) == L"abcdef");
        CHECK(overall_progress(0, 11, 0) == 0);
        CHECK(overall_progress(5, 10, 50) == 55);
        CHECK(overall_progress(10, 10, 0) == 100);
        CHECK(overall_progress(999, 10, 0) == 100);
        CHECK(overall_progress(1, 0, 0) == 0);
        CHECK(overall_progress(0, 10, 999) == 10);

        CHECK(manual_installer_url(std::nullopt) == "https://github.com/Layviz/WindowsRepairTool/releases/latest");
        const std::optional<release_asset> asset = release_asset{"x.msi", "/Layviz/WindowsRepairTool/releases/download/v1.4.4/x.msi", 123};
        CHECK(manual_installer_url(asset) == "https://github.com/Layviz/WindowsRepairTool/releases/download/v1.4.4/x.msi");

        CHECK(batch_shortcut(0).empty());
        CHECK(batch_shortcut(4).empty());
        if (argc == 2) {
            unsigned mode = 0;
            for (const auto name : {"Simple repair.bat", "Default repair.bat", "Extended repair.bat"}) {
                std::ifstream file(std::string(argv[1]) + "/Batch scripts/" + name, std::ios::binary);
                CHECK(file.is_open());
                std::string actual((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                auto expected = batch_shortcut(++mode);
                actual.erase(std::remove(actual.begin(), actual.end(), '\r'), actual.end());
                expected.erase(std::remove(expected.begin(), expected.end(), '\r'), expected.end());
                CHECK(actual == expected);
            }
        }
        std::cout << "PASS issue regressions: platform, durations, streamed progress, SFC reboot, narrow consoles, taskbar, manual updates, batch shortcuts\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
