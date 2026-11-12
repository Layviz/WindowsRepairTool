#include "../Shared/windows_support.h"
#include "localization.h"
#include <atomic>
#include <chrono>
#include <iostream>

using namespace std;

namespace {
atomic<bool> stop{false}, running{false};

BOOL WINAPI CtrlHandler(DWORD event) {
    if ((event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT) && running.load()) {
        stop.store(true); // The current repair must finish; never terminate it.
        return TRUE;
    }
    return FALSE;
}

bool confirm(const wstring& question) {
    wcout << question << L" [y/j = yes, anything else = no] " << flush;
    string input;
    return getline(cin, input) && (input == "y" || input == "Y" || input == "j" || input == "J");
}

void print_warning(const wstring& warning) {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    int columns = 0;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        columns = info.srWindow.Right - info.srWindow.Left + 1;
    const auto line = L"|     " + warning + L"     |";
    const wstring indent(wrt::console_padding(columns, line.size()), L' ');
    const auto border = L"+" + wstring(line.size() - 2, L'-') + L"+";
    wcout << indent << border << endl << indent << line << endl << indent << border << endl;
}

bool allow_step(wrt::risk risk) {
    switch (risk) {
    case wrt::risk::cleanup:
        return confirm(L"Optional cleanup can permanently delete temporary files and, in Standard/Extended mode, empty the Recycle Bin on all drives. Run cleanup?");
    case wrt::risk::surface_scan:
        return confirm(L"Optional full disk surface scan resets the bad-cluster list, may take many hours and may require a reboot. Back up important data first. Run it?");
    case wrt::risk::offline_repair:
        return confirm(L"Optional offline disk repair may require a reboot. Back up important data first. Run it?");
    default: return true;
    }
}

void print_help() {
    wstring help = help_text_fmt;
    auto batch_section = help.find(L"\n\n There are batch files");
    if (batch_section == wstring::npos) batch_section = help.find(L"\n\n Es liegen in");
    if (batch_section != wstring::npos) help.erase(batch_section);
    wcout << help << endl;
    wcout << L"Batch scripts in " << (wrt::install_directory() / L"Batch scripts").wstring()
        << L" are shortcuts to WRT.exe and require the installed binaries. Cleanup and offline disk checks require separate confirmation."
        << endl;
}

int run(int argc, char** argv) {
    load_localized_strings();
    wrt::unique_handle mutex(CreateMutexW(nullptr, FALSE, L"Local\\WRT"));
    const DWORD mutex_error = GetLastError();
    if (!mutex) { wcerr << L"Cannot create application lock: " << mutex_error << endl; return 1; }
    if (mutex_error == ERROR_ALREADY_EXISTS) { wcerr << mutex_warn << endl; return 1; }

    OPERATION requested{};
    if (argc > 2 || (argc == 2 && (!wrt::parse_operation(argv[1], requested) ||
        requested.mode < SIMPLE_REPAIR || requested.mode > EXT_REPAIR))) {
        wcerr << L"Usage: WRT.exe [1|2|3][+|-]" << endl;
        return 2;
    }
    const auto windows = wrt::windows_directory();
    const auto pending = windows / L"WinSxS" / L"pending.xml";
    const DWORD pending_attributes = GetFileAttributesW(pending.c_str());
    if (pending_attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) {
            wcerr << L"Cannot check pending Windows servicing: " << error << endl;
            return 1;
        }
    }
    const bool pending_restart = pending_attributes != INVALID_FILE_ATTRIBUTES;
    if (pending_restart) {
        wcout << pending_query << endl << pending_option1 << endl << pending_option2 << endl;
        string input;
        do {
            if (!getline(cin, input)) return 0;
            if (input == "h") wcout << pending_help_text << endl;
        } while (input == "h");
        if (input == "1") {
            const auto result = wrt::run_attached(wrt::system_program(L"shutdown.exe"), L"/r /t 0");
            return result.completed && result.exit_code == 0 && !result.error ? 0 : 1;
        }
        if (input != "2") return 0;
        wcout << L"DISM steps will be skipped until Windows has restarted." << endl;
    }
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
    if (const auto console = GetConsoleWindow())
        DeleteMenu(GetSystemMenu(console, FALSE), SC_CLOSE, MF_BYCOMMAND);
    print_warning(startup_warn);
    bool global_verbose = false;
    int overall_exit = 0;
    while (!stop.load()) {
        OPERATION operation{};
        if (argc == 2) operation = requested;
        else {
            wcout << endl << mode_query << endl << mode_option1 << endl << mode_option2
                << endl << mode_option3 << endl << mode_cancel << endl << L" " << flush;
            string input;
            if (!getline(cin, input)) break;
            if (!wrt::parse_operation(input, operation)) {
                wcerr << L"Invalid selection." << endl;
                continue;
            }
        }
        if (operation.mode == HELP) { print_help(); continue; }
        if (operation.mode == VERBOSE_ON) { global_verbose = true; continue; }
        if (operation.mode == VERBOSE_OFF) { global_verbose = false; continue; }
        if (operation.mode == NO_OP) break;
        const bool verbose = wrt::effective_verbose(operation.verbose, global_verbose);
        const auto steps = wrt::repair_steps(operation.mode, windows.root_name().wstring());
        size_t failed = 0, skipped = 0;
        bool reboot_required = false, offline_attempted = false;
        const auto start = chrono::steady_clock::now();
        running.store(true);
        wcout << in_progress_note << endl;
        for (size_t i = 0; i < steps.size() && !stop.load(); ++i) {
            const auto& step = steps[i];
            if ((pending_restart && step.program == wrt::tool::dism) || !allow_step(step.confirmation)) {
                ++skipped;
                wcout << L"Skipped step " << i + 1 << L"/" << steps.size() << endl;
                continue;
            }
            if (stop.load()) break;
            const auto executable = wrt::program_path(step.program);
            wcout << L"[" << i + 1 << L"/" << steps.size() << L"] " << executable.filename().wstring()
                << L" " << step.arguments << endl;
            wrt::output_decoder decoder(step.program);
            string diagnostic;
            const auto show_output = [&](string_view text) {
                if (verbose) { cout.write(text.data(), static_cast<streamsize>(text.size())); cout.flush(); }
                diagnostic.append(text.data(), text.size());
                if (diagnostic.size() > 8192) diagnostic.erase(0, diagnostic.size() - 8192);
            };
            const bool offline = step.confirmation == wrt::risk::surface_scan ||
                step.confirmation == wrt::risk::offline_repair;
            // Offline CHKDSK prompts in Windows' own language. Keep its native
            // console instead of injecting a localized UTF-16 answer into stdin.
            const auto result = offline ? wrt::run_attached(executable, step.arguments) :
                wrt::run_process(executable, step.arguments, [&](string_view bytes) {
                    show_output(decoder.decode(bytes));
                });
            show_output(decoder.finish());
            const auto status = wrt::classify_result(step.program, result);
            if (status == wrt::step_status::failed) {
                ++failed;
                wcerr << L"Step failed (Windows error " << result.error << L", exit code "
                    << result.exit_code << L")." << endl;
                if (!verbose) cerr.write(diagnostic.data(), static_cast<streamsize>(diagnostic.size()));
            } else {
                wcout << L"Step completed (exit code " << result.exit_code << L")." << endl;
                reboot_required |= status == wrt::step_status::reboot_required;
                offline_attempted |= offline;
            }
        }
        running.store(false);
        if (stop.load()) wcout << abort_msg << endl;
        const auto seconds = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count();
        wcout << L"Duration: " << seconds << L" " << seconds_str << L". Failed: " << failed
            << L". Skipped: " << skipped << L"." << endl;
        if (failed || stop.load()) overall_exit = 1;
        if (!stop.load() && (reboot_required || offline_attempted)) {
            wcout << L"A reboot may be needed. An offline check runs only if Windows confirmed scheduling it." << endl;
            if (confirm(reboot_query)) {
                const auto result = wrt::run_attached(wrt::system_program(L"shutdown.exe"), L"/r /t 0");
                return result.completed && result.exit_code == 0 && !result.error ? overall_exit : 1;
            }
        }
        if (argc == 2) break;
    }
    return overall_exit;
}
} // namespace

int main(int argc, char** argv) {
    try { return run(argc, argv); }
    catch (const exception& error) { cerr << "WRT error: " << error.what() << endl; return 1; }
}
