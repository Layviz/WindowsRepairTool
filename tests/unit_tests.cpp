#include "core.h"
#include "update.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition); } while (false)

using namespace wrt;
using json = nlohmann::json;

namespace {
json sample_release() {
    return json::parse(R"({"tag_name":"v1.4.4","assets":[
        {"name":"checksums.txt","browser_download_url":"ignored","size":128},
        {"name":"WRT_Installer-Version1.4.4.msi","size":4096,
         "browser_download_url":"https://github.com/Layviz/WindowsRepairTool/releases/download/v1.4.4/WRT_Installer-Version1.4.4.msi"}]})");
}

bool transfer(const std::string& input, std::string& output, std::uint64_t limit,
    std::optional<std::uint64_t> expected = std::nullopt, bool read_error = false, bool write_error = false) {
    std::size_t position = 0;
    return transfer_body([&](char* buffer, std::size_t capacity, std::size_t& count) {
        if (read_error && position > 0) return false;
        count = std::min({capacity, input.size() - position, std::size_t(512)});
        std::memcpy(buffer, input.data() + position, count);
        position += count;
        return true;
    }, [&](std::string_view bytes) {
        if (write_error) return false;
        output.append(bytes.data(), bytes.size());
        return true;
    }, limit, expected);
}

const std::vector<std::pair<std::string, std::function<void()>>> cases = {
    {"input", [] {
        OPERATION op{};
        CHECK(parse_operation("1", op) && op.mode == SIMPLE_REPAIR && op.verbose == DEFAULT);
        CHECK(parse_operation("2+", op) && op.mode == DEFAULT_REPAIR && op.verbose == VERBOSE);
        CHECK(parse_operation("3-", op) && op.mode == EXT_REPAIR && op.verbose == SILENT);
        CHECK(parse_operation("h", op) && op.mode == HELP);
        CHECK(parse_operation("?", op) && op.mode == HELP);
        CHECK(parse_operation("+", op) && op.mode == VERBOSE_ON);
        CHECK(parse_operation("-", op) && op.mode == VERBOSE_OFF);
        CHECK(parse_operation("", op) && op.mode == NO_OP && op.verbose == DEFAULT);
        CHECK(parse_operation("q", op) && op.mode == NO_OP);
    }},
    {"input_invalid", [] {
        for (const auto input : {"1+extra", "2garbage", "h+", "++", "1 ", "12", " 1", "3--", "1\r"}) {
            OPERATION op{};
            CHECK(!parse_operation(input, op));
        }
        OPERATION op{};
        CHECK(!parse_operation(std::string_view("1\0", 2), op));
    }},
    {"verbosity", [] {
        for (bool global : {false, true}) {
            CHECK(!effective_verbose(SILENT, global));
            CHECK(effective_verbose(VERBOSE, global));
            CHECK(effective_verbose(DEFAULT, global) == global);
        }
    }},
    {"padding", [] {
        CHECK(console_padding(40, 72) == 0);
        CHECK(console_padding(72, 72) == 0);
        CHECK(console_padding(80, 72) == 4);
        CHECK(console_padding(-1, 72) == 0);
        CHECK(console_padding(0, 72) == 0);
        CHECK(console_padding(80, std::numeric_limits<std::size_t>::max()) == 0);
    }},
    {"language", [] {
        CHECK(language_name(L"de-DE") == L"de");
        CHECK(language_name(L"DE-at") == L"de");
        CHECK(language_name(L"en-US") == L"en");
        CHECK(language_name(L"zh-Hans-CN") == L"zh");
        CHECK(language_name(L"fil-PH") == L"fil");
        for (const auto input : {L"", L"../de", L"de\\x", L"d", L"123", L"de_DE"})
            CHECK(language_name(input).empty());
    }},
    {"quoting", [] {
        CHECK(quote_argument(L"") == L"\"\"");
        CHECK(quote_argument(L"C:\\Program Files\\WRT\\") == L"\"C:\\Program Files\\WRT\\\\\"");
        CHECK(quote_argument(L"a\"b") == L"\"a\\\"b\"");
        CHECK(quote_argument(L"a\\\"b") == L"\"a\\\\\\\"b\"");
        CHECK(quote_argument(L"a&b") == L"\"a&b\"");
    }},
    {"versions", [] {
        CHECK(parse_version("v1.4.3")->packed() == 0x01040003u);
        CHECK(parse_version("1.4.4")->packed() > parse_version("1.4.3")->packed());
        CHECK(parse_version("2.0.0")->packed() > parse_version("1.255.65535")->packed());
        CHECK(parse_version("255.255.65535")->packed() == 0xffffffffu);
        CHECK(parse_version("0.0.0")->packed() == 0);
    }},
    {"versions_invalid", [] {
        for (const auto input : {"", "v", "vNext", "1", "1.2", "1.2.", ".1.2", "1..2", "1.2.3.4",
            "-1.2.3", "1.2.3-beta", "1.2.3x", " 1.2.3", "256.0.0", "0.256.0", "0.0.65536",
            "999999999999999999999.1.2"}) CHECK(!parse_version(input));
    }},
    {"output_bounds", [] {
        output_decoder dism(tool::dism);
        std::array<char, 512> full;
        full.fill('A'); // Deliberately no terminating zero; the original ASan failure.
        CHECK(dism.decode({full.data(), full.size()}) == std::string(512, 'A'));
        CHECK(dism.decode(std::string_view("a\0b", 3)) == std::string("a\0b", 3));
        CHECK(dism.decode({}).empty());
    }},
    {"output_utf16", [] {
        output_decoder sfc(tool::sfc);
        CHECK(sfc.decode(std::string_view("\xff\xfe" "A\0\xe4\0\x2c\x20\x3d\xd8\x00\xde", 12)) ==
            "A\xc3\xa4\xe2\x80\xac\xf0\x9f\x98\x80");
        CHECK(sfc.decode(std::string_view("\r\0\n\0", 4)) == "\r\n");
    }},
    {"output_split", [] {
        const std::string bytes("\xff\xfe" "A\0\xe4\0\x3d\xd8\x00\xde", 10);
        const std::string expected = "A\xc3\xa4\xf0\x9f\x98\x80";
        for (std::size_t split = 0; split <= bytes.size(); ++split) {
            output_decoder sfc(tool::sfc);
            const auto first = sfc.decode(std::string_view(bytes).substr(0, split));
            CHECK(first + sfc.decode(std::string_view(bytes).substr(split)) == expected);
        }
        output_decoder sfc(tool::sfc);
        std::string output;
        for (char byte : bytes) output += sfc.decode(std::string_view(&byte, 1));
        CHECK(output == expected);
    }},
    {"output_malformed", [] {
        output_decoder sfc(tool::sfc);
        CHECK(sfc.decode(std::string_view("\0\xdc", 2)) == "\xef\xbf\xbd");
        CHECK(sfc.decode(std::string_view("\0\xd8" "A\0", 4)) == "\xef\xbf\xbd" "A");
        CHECK(sfc.decode(std::string_view("\0\xd8", 2)).empty());
        CHECK(sfc.finish() == "\xef\xbf\xbd");
        CHECK(sfc.decode("A").empty());
        CHECK(sfc.finish() == "\xef\xbf\xbd");
        CHECK(sfc.finish().empty());
    }},
    {"repair_modes", [] {
        CHECK(repair_steps(SIMPLE_REPAIR, L"D:").size() == 11);
        CHECK(repair_steps(DEFAULT_REPAIR, L"D:").size() == 11);
        CHECK(repair_steps(EXT_REPAIR, L"D:").size() == 5);
        CHECK(repair_steps(HELP, L"D:").empty());
        for (auto mode : {SIMPLE_REPAIR, DEFAULT_REPAIR, EXT_REPAIR}) {
            for (const auto& step : repair_steps(mode, L"D:")) CHECK(step.arguments.find(L"C:") == std::wstring::npos);
        }
    }},
    {"repair_risks", [] {
        for (auto mode : {SIMPLE_REPAIR, DEFAULT_REPAIR, EXT_REPAIR}) {
            for (const auto& step : repair_steps(mode, L"C:")) {
                if (step.program == tool::cleanup) CHECK(step.confirmation == risk::cleanup);
                if (step.arguments.find(L" /b") != std::wstring::npos) {
                    CHECK(step.confirmation == risk::surface_scan);
                    CHECK(step.arguments.find(L"/scan") == std::wstring::npos);
                }
                if (step.arguments.find(L"/spotfix") != std::wstring::npos)
                    CHECK(step.confirmation == risk::offline_repair);
            }
        }
    }},
    {"result_status", [] {
        CHECK(classify_result(tool::sfc, {}) == step_status::failed);
        CHECK(classify_result(tool::sfc, {true, false, 0, 0}) == step_status::failed);
        CHECK(classify_result(tool::sfc, {true, true, 0, 6}) == step_status::failed);
        CHECK(classify_result(tool::sfc, {true, true, 0, 0}) == step_status::success);
        CHECK(classify_result(tool::sfc, {true, true, 1, 0}) == step_status::failed);
        CHECK(classify_result(tool::chkdsk, {true, true, 1, 0}) == step_status::success);
        CHECK(classify_result(tool::chkdsk, {true, true, 2, 0}) == step_status::failed);
        CHECK(classify_result(tool::chkdsk, {true, true, 3, 0}) == step_status::failed);
        CHECK(classify_result(tool::dism, {true, true, 3010, 0}) == step_status::reboot_required);
        CHECK(classify_result(tool::defender, {true, true, 2, 0}) == step_status::failed);
    }},
    {"assets", [] {
        const auto asset = installer_asset(sample_release());
        CHECK(asset && asset->name == "WRT_Installer-Version1.4.4.msi" && asset->size == 4096);
        CHECK(asset->request_path == "/Layviz/WindowsRepairTool/releases/download/v1.4.4/WRT_Installer-Version1.4.4.msi");
    }},
    {"asset_rejection", [] {
        for (const auto name : {"", "x", ".msi", "../x.msi", "a\\x.msi", "x&calc.msi", "x.msi.exe", "x\".msi", "x y.msi"})
            CHECK(!safe_msi_name(name));
        for (const auto url : {"http://github.com/x.msi", "https://evil.example/x.msi",
            "https://github.com.evil.example/x.msi", "https://github.com/Other/Repo/releases/download/v1.4.4/x.msi"}) {
            auto release = sample_release();
            release["assets"][1]["browser_download_url"] = url;
            CHECK(!installer_asset(release));
        }
        for (const auto& bad : {json(), json::array(), json{{"assets", 1}}, json{{"tag_name", 1}}}) CHECK(!installer_asset(bad));
        auto release = sample_release();
        release["assets"].push_back(release["assets"][1]);
        CHECK(!installer_asset(release));
        release = sample_release();
        release["assets"][1]["size"] = 0u;
        CHECK(!installer_asset(release));
        release["assets"][1]["size"] = 600u * 1024 * 1024;
        CHECK(!installer_asset(release));
    }},
    {"update_gate", [] {
        for (bool download : {false, true}) for (bool verify : {false, true}) for (bool install : {false, true}) {
            std::string calls;
            const bool result = perform_update([&] { calls += 'D'; return download; },
                [&] { calls += 'V'; return verify; }, [&] { calls += 'I'; return install; });
            CHECK(result == (download && verify && install));
            CHECK(calls == (!download ? "D" : !verify ? "DV" : "DVI"));
        }
    }},
    {"transfer", [] {
        std::string output;
        const std::string input(65537, 'x');
        CHECK(transfer(input, output, input.size(), input.size()));
        CHECK(output == input);
        output.clear();
        CHECK(transfer(std::string("a\0b", 3), output, 3));
        CHECK(output == std::string("a\0b", 3));
    }},
    {"transfer_failure", [] {
        std::string output;
        CHECK(!transfer("", output, 100));
        CHECK(!transfer("abc", output, 0));
        CHECK(!transfer("abc", output, 100, 0));
        CHECK(!transfer("abc", output, 100, 4));
        CHECK(!transfer("abcd", output, 100, 3));
        CHECK(!transfer("abc", output, 2));
        CHECK(!transfer("abc", output, 2, 3));
        CHECK(!transfer(std::string(1024, 'x'), output, 1024, 1024, true));
        CHECK(!transfer("abc", output, 100, 3, false, true));
        CHECK(!transfer_body([](char*, std::size_t capacity, std::size_t& count) {
            count = capacity + 1; return true;
        }, [](std::string_view) { return true; }, 100000));
    }},
    {"installer_arguments", [] {
        const auto arguments = installer_arguments(L"C:\\WRT Update\\installer.msi", L"C:\\Program Files\\Windows Repair Tool");
        CHECK(arguments == L"/i \"C:\\WRT Update\\installer.msi\" TARGETDIR=\"C:\\Program Files\\Windows Repair Tool\" /passive /norestart");
        CHECK(arguments.find(L"/x") == std::wstring::npos);
        CHECK(arguments.find(L"cmd") == std::wstring::npos);
    }},
    {"installer_exit", [] {
        for (auto code : {0u, 3010u, 1641u}) CHECK(successful_install_exit(code));
        for (auto code : {1u, 1603u, 1618u}) CHECK(!successful_install_exit(code));
    }}
};
} // namespace

int main(int argc, char** argv) {
    std::size_t executed = 0;
    for (const auto& test : cases) {
        if (argc > 1 && test.first != argv[1]) continue;
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& error) { std::cerr << "FAIL " << test.first << ": " << error.what() << '\n'; return 1; }
        ++executed;
    }
    return executed ? 0 : 2;
}
