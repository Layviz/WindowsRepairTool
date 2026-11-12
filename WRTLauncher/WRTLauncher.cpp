#include "../Shared/windows_update.h"
#include "../Shared/windows_issues.h"
#include "../version.h"
#include "localization.h"
#include <iostream>

using namespace std;
using json = nlohmann::json;

namespace {
int run() {
    load_localized_strings();
    wrt::restore_console(true);
    if (!wrt::native_platform_supported()) {
        wcerr << wrt::issue_text(ISSUE_PLATFORM_ERROR) << endl;
        return 1;
    }
    wrt::unique_handle mutex;
    if (!wrt::wait_for_instance(mutex)) return 0;
    const auto root = wrt::install_directory();
    if (const auto error = wrt::ensure_batch_shortcuts(root))
        wcerr << wrt::format(wrt::issue_text(ISSUE_BATCH_ERROR), {error}) << endl;
    wcout << wrt::format(wrt::issue_text(ISSUE_CURRENT_VERSION),
        {VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH}) << endl;
    string response;
    const bool received = wrt::http_get(L"api.github.com", L"/repos/Layviz/WindowsRepairTool/releases/latest",
        [&](string_view bytes) { response.append(bytes.data(), bytes.size()); return true; }, 1024 * 1024);
    optional<wrt::release_asset> asset;
    optional<wrt::release_asset> latest_installer;
    if (received) {
        const auto release = json::parse(response, nullptr, false);
        if (release.is_object() && release.contains("tag_name") && release["tag_name"].is_string()) {
            const auto version = wrt::parse_version(release["tag_name"].get<string>());
            latest_installer = wrt::installer_asset(release);
            if (version && version->packed() > CURRENT_VERSION) {
                asset = latest_installer;
                if (!asset) wcerr << error_download_url << endl;
                else wcout << wrt::format(new_version_fmt,
                    {version->major, version->minor, version->patch}) << endl;
            } else if (version) wcout << (version->packed() == CURRENT_VERSION ? version_up_to_date : version_ahead) << endl;
            else wcerr << version_unknown << endl;
        } else wcerr << version_unknown << endl;
    } else wcerr << error_receive << endl;

    const auto manual_url = wrt::manual_installer_url(latest_installer);
    const wstring manual_link(manual_url.begin(), manual_url.end());
    const auto show_manual_link = [&] {
        wcout << wrt::format(wrt::issue_text(ISSUE_MANUAL_DOWNLOAD), {manual_link}) << endl;
    };
    show_manual_link(); // Available even when the API, verification or automatic update fails.

    if (asset) {
        wcout << endl << wrt::issue_text(ISSUE_UPDATE_HINT) << endl;
        string input;
        do {
            wcout << endl << install_query << endl << install_mode1 << endl << install_mode2 << endl
                << install_mode3 << endl << wrt::issue_text(ISSUE_MANUAL_OPTION) << endl << L"> " << flush;
            if (!getline(cin, input) || input == "3") return 0;
            if (input == "4") show_manual_link();
        } while (input != "1" && input != "2");
        if (input == "1") {
            // Fail closed for unsigned builds before downloading anything.
            const auto publisher = wrt::trusted_signer(wrt::module_path());
            wrt::installer_download download;
            wrt::process_result installed;
            const bool success = !publisher.empty() && wrt::perform_update(
                [&] { return download.download(*asset); },
                [&] { return wrt::trusted_signer(download.file()) == publisher; },
                [&] {
                    const auto arguments = wrt::installer_arguments(download.file().wstring(), root.wstring());
                    wcout << installing_fmt << endl;
                    installed = wrt::run_attached(wrt::system_program(L"msiexec.exe"), arguments, {}, true);
                    return installed.completed && !installed.error && wrt::successful_install_exit(installed.exit_code);
                });
            if (!success) {
                wcerr << error_update << endl << update_manual_hint << endl;
                show_manual_link();
                // A failed update must not prevent access to the installed repair tool.
            } else if (installed.exit_code != 0) {
                wcout << wrt::format(update_reboot_fmt, {installed.exit_code}) << endl;
                return 0;
            }
        }
    }
    mutex.reset();
    wrt::restore_console(); // The minimized installer must not minimize the repair UI.
    const auto result = wrt::run_attached(root / L"Binaries" / L"WRT.exe", L"", root);
    if (!result.started || !result.completed || result.error) {
        wcerr << error_wrt << L"Windows error " << result.error << endl;
        return 1;
    }
    return static_cast<int>(result.exit_code);
}
} // namespace

int main() {
    try { return run(); }
    catch (const exception& error) { cerr << "Launcher error: " << error.what() << endl; return 1; }
}
