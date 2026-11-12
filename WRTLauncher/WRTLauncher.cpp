#include "../Shared/windows_update.h"
#include "../version.h"
#include "localization.h"
#include <iostream>

using namespace std;
using json = nlohmann::json;

namespace {
int run() {
    load_localized_strings();
    wrt::unique_handle mutex(CreateMutexW(nullptr, FALSE, L"Local\\WRT"));
    const DWORD mutex_error = GetLastError();
    if (!mutex) { wcerr << L"Cannot create application lock: " << mutex_error << endl; return 1; }
    if (mutex_error == ERROR_ALREADY_EXISTS) { wcerr << mutex_warn << endl; return 1; }
    const auto root = wrt::install_directory();
    string response;
    const bool received = wrt::http_get(L"api.github.com", L"/repos/Layviz/WindowsRepairTool/releases/latest",
        [&](string_view bytes) { response.append(bytes.data(), bytes.size()); return true; }, 1024 * 1024);
    optional<wrt::release_asset> asset;
    if (received) {
        const auto release = json::parse(response, nullptr, false);
        if (release.is_object() && release.contains("tag_name") && release["tag_name"].is_string()) {
            const auto version = wrt::parse_version(release["tag_name"].get<string>());
            if (version && version->packed() > CURRENT_VERSION) {
                asset = wrt::installer_asset(release);
                if (!asset) wcerr << error_download_url << endl;
                else wcout << L"New version: " << version->major << L"." << version->minor << L"." << version->patch << endl;
            } else if (version) wcout << (version->packed() == CURRENT_VERSION ? version_up_to_date : version_ahead) << endl;
            else wcerr << version_unknown << endl;
        } else wcerr << version_unknown << endl;
    } else wcerr << error_receive << endl;

    if (asset) {
        wcout << install_query << endl << install_mode1 << endl << install_mode2 << endl << install_mode3 << endl;
        string input;
        if (!getline(cin, input) || input == "3") return 0;
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
                    wcout << L"Installing verified update (no separate uninstall)." << endl;
                    installed = wrt::run_attached(wrt::system_program(L"msiexec.exe"), arguments);
                    return installed.completed && !installed.error && wrt::successful_install_exit(installed.exit_code);
                });
            if (!success) {
                wcerr << error_update << endl
                    << L"The existing installation was not explicitly uninstalled. Automatic updates require a trusted signature using the launcher's signing certificate."
                    << endl << L"For unsigned builds, install a release manually from https://github.com/Layviz/WindowsRepairTool/releases ." << endl;
                // A failed update must not prevent access to the installed repair tool.
            } else if (installed.exit_code != 0) {
                wcout << L"Update installed; Windows requires a restart (exit code " << installed.exit_code << L")." << endl;
                return 0;
            }
        } else if (input != "2") return 0;
    }
    mutex.reset();
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
