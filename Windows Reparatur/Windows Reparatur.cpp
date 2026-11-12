#include "../Shared/windows_feedback.h"
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
    wcout << question << confirm_hint << flush;
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
        return confirm(confirm_cleanup);
    case wrt::risk::surface_scan:
        return confirm(confirm_surface_scan);
    case wrt::risk::offline_repair:
        return confirm(confirm_offline_repair);
    default: return true;
    }
}

void print_help() {
    wcout << wrt::issue_text(ISSUE_HELP) << endl;
    wcout << wrt::format(help_batch_hint, {(wrt::install_directory() / L"Batch scripts").wstring()}) << endl;
}

int run(int argc, char** argv) {
    load_localized_strings();
    wrt::restore_console(true);
    if (!wrt::native_platform_supported()) {
        wcerr << wrt::issue_text(ISSUE_PLATFORM_ERROR) << endl;
        return 1;
    }
    wrt::unique_handle mutex;
    if (!wrt::wait_for_instance(mutex)) return 0;
    wrt::repair_console console;
    wrt::taskbar_progress taskbar;
    wrt::desktop_notifications notifications;
    // Logging is optional: a missing or unwritable log file simply disables it.
    wrt::step_logger log;
    if (const auto logs = wrt::program_data_logs_dir()) log = wrt::step_logger(*logs / L"WRT.log");

    OPERATION requested{};
    if (argc > 2 || (argc == 2 && (!wrt::parse_operation(argv[1], requested) ||
        requested.mode < SIMPLE_REPAIR || requested.mode > EXT_REPAIR))) {
        wcerr << usage_text << endl;
        return 2;
    }
    const auto windows = wrt::windows_directory();
    const auto pending = windows / L"WinSxS" / L"pending.xml";
    const DWORD pending_attributes = GetFileAttributesW(pending.c_str());
    if (pending_attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) {
            wcerr << wrt::format(pending_check_error_fmt, {error}) << endl;
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
        wcout << dism_skipped_note << endl;
    }
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
    if (const auto console = GetConsoleWindow())
        DeleteMenu(GetSystemMenu(console, FALSE), SC_CLOSE, MF_BYCOMMAND);
    wcout << console.color(L"\x1b[93m");
    print_warning(startup_warn);
    wcout << console.color(L"\x1b[0m");
    bool global_verbose = false;
    int overall_exit = 0;
    while (!stop.load()) {
        OPERATION operation{};
        if (argc == 2) operation = requested;
        else {
            wcout << console.color(L"\x1b[96m") << endl << mode_query << endl << wrt::issue_text(ISSUE_MODE_SIMPLE)
                << endl << mode_option2 << endl << mode_option3 << console.color(L"\x1b[0m")
                << endl << mode_cancel << endl << L"> " << flush;
            string input;
            if (!getline(cin, input)) break;
            if (!wrt::parse_operation(input, operation)) {
                wcerr << invalid_selection << endl;
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
        notifications.show(wrt::issue_text(ISSUE_NOTIFY_STARTED));
        taskbar.update(0, steps.size());
        log.line(L"Repair started (mode " + std::to_wstring(static_cast<unsigned>(operation.mode))
            + L", " + std::to_wstring(steps.size()) + L" steps).");
        for (size_t i = 0; i < steps.size() && !stop.load(); ++i) {
            taskbar.update(i, steps.size());
            const auto& step = steps[i];
            if ((pending_restart && step.program == wrt::tool::dism) || !allow_step(step.confirmation)) {
                ++skipped;
                wcout << wrt::format(step_skipped_fmt, {i + 1, steps.size()}) << endl;
                log.line(L"Step " + std::to_wstring(i + 1) + L"/" + std::to_wstring(steps.size()) + L" skipped.");
                continue;
            }
            if (stop.load()) break;
            const auto executable = wrt::program_path(step.program);
            const auto program_name = executable.filename().wstring();
            // Offline CHKDSK prompts in Windows' own language. Keep its native
            // console instead of injecting a localized UTF-16 answer into stdin;
            // while it runs, the child owns the console, so no in-place redraw.
            const bool offline = step.confirmation == wrt::risk::surface_scan ||
                step.confirmation == wrt::risk::offline_repair;
            const bool in_place = !verbose && !offline && console.columns() > 1;
            const auto status_text = wrt::format(step_status_fmt,
                {i + 1, steps.size(), program_name, step.arguments});
            std::size_t line_width = 0;
            if (in_place) console.redraw(wcout, status_text, line_width);
            else wcout << console.color(L"\x1b[96m") << status_text << console.color(L"\x1b[0m") << endl;
            log.line(L"Step " + std::to_wstring(i + 1) + L"/" + std::to_wstring(steps.size())
                + L": " + program_name + L" " + step.arguments);
            wrt::output_decoder decoder(step.program);
            wrt::progress_reader progress;
            wrt::reboot_reader sfc_reboot;
            string diagnostic;
            unsigned shown_percent = 0;
            bool has_percent = false;
            const auto redraw = [&](unsigned percent) {
                console.redraw(wcout, wrt::format(step_progress_fmt,
                    {i + 1, steps.size(), program_name, percent}), line_width);
            };
            const auto show_output = [&](string_view text) {
                if (verbose) { cout.write(text.data(), static_cast<streamsize>(text.size())); cout.flush(); }
                diagnostic.append(text.data(), text.size());
                if (diagnostic.size() > 8192) diagnostic.erase(0, diagnostic.size() - 8192);
                if (step.program == wrt::tool::sfc) sfc_reboot.feed(text);
                if (const auto percent = progress.feed(text);
                    percent && (!has_percent || *percent != shown_percent)) {
                    shown_percent = *percent;
                    has_percent = true;
                    if (in_place) redraw(shown_percent);
                    taskbar.update(i, steps.size(), shown_percent);
                }
            };
            const auto step_start = chrono::steady_clock::now();
            if (verbose) wcout << wrt::format(wrt::issue_text(ISSUE_STEP_STARTED),
                {program_name}) << L" " << wrt::timestamp() << endl;
            const auto result = offline ? wrt::run_attached(executable, step.arguments) :
                wrt::run_process(executable, step.arguments, [&](string_view bytes) {
                    show_output(decoder.decode(bytes));
                });
            show_output(decoder.finish());
            const auto status = wrt::classify_result(step.program, result);
            if (in_place) wcout << endl; // Keep the last progress line, then continue below it.
            if (verbose) {
                // Tool output is not guaranteed to end with a newline.
                wcout << endl << wrt::timestamp() << L" " << wrt::format(wrt::issue_text(ISSUE_STEP_TIME),
                    {program_name, wrt::elapsed_seconds(chrono::duration_cast<chrono::milliseconds>(
                        chrono::steady_clock::now() - step_start)), seconds_str}) << endl;
            }
            if (step.program == wrt::tool::sfc && (sfc_reboot.required() || wrt::pending_reboot())) {
                reboot_required = true;
                wcout << console.color(L"\x1b[93m") << wrt::issue_text(ISSUE_SFC_REBOOT)
                    << console.color(L"\x1b[0m") << endl;
                log.line(L"SFC/Windows reports a pending restart.");
            }
            if (status == wrt::step_status::failed) {
                ++failed;
                wcerr << console.color(L"\x1b[91m", true) << wrt::format(step_failed_fmt, {result.error, result.exit_code})
                    << console.color(L"\x1b[0m", true) << endl;
                taskbar.error();
                if (!verbose) cerr.write(diagnostic.data(), static_cast<streamsize>(diagnostic.size()));
                log.line(L"Step failed (Windows error " + std::to_wstring(result.error) + L", exit code "
                    + std::to_wstring(result.exit_code) + L").");
                log.dump(diagnostic);
            } else {
                wcout << console.color(L"\x1b[92m") << wrt::format(step_done_fmt, {result.exit_code})
                    << console.color(L"\x1b[0m") << endl;
                reboot_required |= status == wrt::step_status::reboot_required;
                offline_attempted |= offline;
                log.line(L"Step completed (exit code " + std::to_wstring(result.exit_code) + L").");
            }
        }
        running.store(false);
        notifications.show(wrt::issue_text(failed || stop.load() ? ISSUE_NOTIFY_FAILED : ISSUE_NOTIFY_DONE));
        taskbar.clear();
        const auto seconds = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count();
        if (stop.load()) {
            wcout << abort_msg << endl;
            log.line(L"Repair aborted by user.");
        }
        wcout << wrt::format(summary_fmt, {seconds, seconds_str, failed, skipped}) << endl;
        log.line(L"Repair finished in " + std::to_wstring(seconds) + L" s (failed: "
            + std::to_wstring(failed) + L", skipped: " + std::to_wstring(skipped) + L").");
        if (failed || stop.load()) overall_exit = 1;
        if (!stop.load() && (reboot_required || offline_attempted)) {
            wcout << reboot_hint << endl;
            if (confirm(reboot_query)) {
                log.line(L"Reboot requested.");
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
