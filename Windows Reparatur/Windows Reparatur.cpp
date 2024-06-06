#include <iostream>
#include <Windows.h>
#include <PathCch.h>
#include <string>
#include <time.h>
#include "resource.h"
#include "localization.h"
#include "wrt_types.h"
#include "terminal_interface.h"

using namespace std;


int total;
int counter;
int start_process(wstring command, PROCESS_INFORMATION* proc_info,HANDLE * input,HANDLE * output);
void wait_for_process(PROCESS_INFORMATION* proc_info);
void create_time_str(wchar_t* buffer, size_t size, long long sec);
void print_help();
bool verbose = false;   //verbose for all runs
bool stop = false, running = false;
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);


int main()
{
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
        // ignore this, not much to do about it
    }
    setup_ui();
    load_localized_strings();
    wstring simpleRepair[] = {
        L"defrag C: /o /h",
        L"sfc /scannow",
        L"mpcmdrun.exe -Scan -ScanType 1",
        L"chkdsk C: /scan /perf /i",
        L"dism /online /cleanup-image /CheckHealth",
        L"Dism /Online /Cleanup-Image /ScanHealth",
        L"Dism /Online /Cleanup-Image /RestoreHealth",
        L"chkdsk C: /scan /perf",
        L"sfc /scannow",
        L"defrag C: /o /h"
    };
    wstring defaultRepair[] = {
        L"defrag C: /o /h",
        L"sfc /scannow",
        L"mpcmdrun.exe -Scan -ScanType 2",
        L"chkdsk C: /scan /perf",
        L"dism /online /cleanup-image /CheckHealth",
        L"Dism /Online /Cleanup-Image /ScanHealth",
        L"Dism /Online /Cleanup-Image /RestoreHealth",
        L"sfc /scannow",
        L"defrag /c /o /h /m",
        L"chkdsk C: /scan /perf /f /r /x /b"
    };

    wstring extendedRepair[] = {
        L"defrag C: /o /h",
        L"chkdsk C: /scan /perf /i",
        L"sfc /scannow",
        L"chkdsk C: /f /x /spotfix /sdcleanup"
    };


    HANDLE mutex = CreateMutex(NULL, false, L"Local\\WRT");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        show_error(mutex_warn, true);
        return -1;
    }
    std::wcout << endl;


    int auswahl = 0;
    // test for pending.xml
    GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml");
    if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml") && GetLastError() != ERROR_FILE_NOT_FOUND) {
    // fragen ob man trotzdem reparieren will
        wchar_t* choices[2] = { pending_option1,pending_option2 };
        auswahl = query_pending_reboot(pending_query, mode_cancel, choices, sizeof(choices), pending_help_text);
        switch (auswahl)
        {
        case 1:
            system("shutdown /r /t 0");
            return 0;
        case 2:
            break;
        default:
            return 0;
        }
    }

    if (!SetConsoleCtrlHandler( CtrlHandler, TRUE)) {
#ifdef DEBUG
        show_error(L"handler not registred\n",false);
#endif
    }

    if (!DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND)) {
#ifdef DEBUG
        show_error(L"close button not removed\n",false);
#endif
    }

    // set time zone
    _tzset();

    // set the path env var to windows defender path
    // system32 is searched by default so no need to include in path

    wchar_t wd_path[MAX_PATH];
    BOOL env_rv = GetEnvironmentVariable(L"ProgramFiles", wd_path, MAX_PATH);
    if (!env_rv) {
        wcscpy_s(wd_path, L"C:\\Program Files");
    }
    wcscat_s(wd_path, L"\\Windows Defender");
    env_rv = SetEnvironmentVariable(L"Path", wd_path);
    if (!env_rv) {
        std::wcout << "ERROR: Failed to set Path variable" << endl;
    }


    show_logo();

    show_warning(startup_warn);

    wchar_t* choices[3] = { mode_option1 ,mode_option2, mode_option3 };
    wchar_t repair_time_str[MAX_LOCALIZED_STRING_SIZE];
    wchar_t start_time_str[MAX_LOCALIZED_STRING_SIZE];
    wchar_t end_time_str[MAX_LOCALIZED_STRING_SIZE];
    wchar_t time_buffer[MAX_LOCALIZED_STRING_SIZE];
    wchar_t done[MAX_LOCALIZED_STRING_SIZE];
    wchar_t* add;
    /*-------------
    *             *
    *  MAIN LOOP  *
    *             *
    *-------------*/
    while (!stop) {
        OPERATION op = {};
        bool r;

        bool verbose_current = false; //verbose for the next run
        r = query_repair_mode(mode_query, choices, sizeof(choices), mode_cancel, &op);

        if (!r) {
            return 0;
        }
        switch (op.mode) {
        case SIMPLE_REPAIR:
        case DEFAULT_REPAIR:
        case EXT_REPAIR:
            break;
        case HELP:
            while (op.mode == HELP) {
                print_help();
                r = read_user_intput(&op);
                if (!r) {
                    break;
                }
                switch (op.mode)
                {
                case SIMPLE_REPAIR:
                case DEFAULT_REPAIR:
                case EXT_REPAIR:
                case NO_OP:
                case HELP:
                    break;
                case VERBOSE_ON:
                    verbose = true;
                    break;
                case VERBOSE_OFF:
                    verbose = false;
                    break;
                }
            }
            break;
        case VERBOSE_ON:
            verbose = true;
            break;
        case VERBOSE_OFF:
            verbose = false;
            break;
        case NO_OP:
            return 0;
            break;
        }

        switch (op.verbose) {
        case VERBOSE:
            verbose_current = true;
            break;
        case SILENT:
            verbose_current = false;
        default:
            verbose_current = verbose;
        }

        switch (op.mode)
        {
        case SIMPLE_REPAIR:
            total = sizeof(simpleRepair) / sizeof(wstring);
            break;
        case DEFAULT_REPAIR:
            total = sizeof(defaultRepair) / sizeof(wstring);
            break;
        case EXT_REPAIR:
            total = sizeof(extendedRepair) / sizeof(wstring);
            break;
        default:
            total = 0;
            break;
        }
        counter = 1;
        time_t repair_start_time,repair_end_time;
        time(&repair_start_time);


        running = true;
        for (int i = 0; i < total; i++)
        {
            if (0 == i) {
                show_in_progress_note(in_progress_note);
            }
            time_t process_start_time,process_end_time;
            time(&process_start_time);
            PROCESS_INFORMATION proc_info;
            HANDLE input = NULL, output = NULL;
            int rv = 0;
            wchar_t timestr[16];
            if (SIMPLE_REPAIR == op.mode)
            {
                rv = start_process(simpleRepair[i],&proc_info,&input,&output);
            }
            else if (DEFAULT_REPAIR == op.mode)
            {
                rv = start_process(defaultRepair[i], &proc_info, &input, &output);
            }
            else if (EXT_REPAIR == op.mode)
            {
                rv = start_process(extendedRepair[i], &proc_info, &input, &output);
            }
            if (rv <= -10) {
                show_error(L"\nERROR on creating pipes for subprocess\n", false);
            }
            else if (rv < 0) {
                show_error(L"\ERROR on creating subprocess\n", false);
                wchar_t error_buffer[1024];
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), error_buffer, 1024, NULL);
                show_error(error_buffer, false);
            }
            else if (0 == rv) {
                add = NULL;
                _wstrtime_s(timestr);
                swprintf(done, MAX_LOCALIZED_STRING_SIZE, progress_started_fmt, counter, total);
                
                if (verbose_current) {
                    swprintf(start_time_str, MAX_LOCALIZED_STRING_SIZE, process_start_time_fmt, timestr);
                    add = start_time_str;
                }
                progress_note(done, add, !verbose_current);
            }
            if (i == total-1) {
                wchar_t confirmation[] = {reboot_confirms[0],'\r','\n','\0'};
                DWORD written = 0;
                WriteFile(input, confirmation, sizeof(confirmation), &written, NULL);
            }
            if (verbose_current && output && rv == 0) {
                char buffer[512] = {};
                DWORD read = 0;
                BOOL success;
                //std::wcout << endl << endl << endl;
                do {
                    success = ReadFile(output, buffer, 512, &read, NULL);
                    if (success) {
                        add_progress_output(buffer, read);
                    }
                    else {
#ifdef DEBUG
                        show_error(L"error on read\n", false);
#endif
                    }
                    memset(buffer, 0, 512);
                } while (success);
                //std::wcout << endl << endl;
            }
            if(input)
                CloseHandle(input);
            if(output)
                CloseHandle(output);
            wait_for_process(&proc_info);
            time(&process_end_time);
            _wstrtime_s(timestr);
            add = NULL;
            swprintf(done, MAX_LOCALIZED_STRING_SIZE, progress_done_fmt, counter++, total);
            if (verbose_current) {
                long long diff_sec = process_end_time - process_start_time;
                create_time_str(time_buffer, MAX_LOCALIZED_STRING_SIZE, diff_sec);
                swprintf(end_time_str, MAX_LOCALIZED_STRING_SIZE, process_end_time_fmt, timestr, time_buffer);
                add = end_time_str;
            }
            progress_note(done, add, !verbose_current);
            if (stop)
                break;
        }
        running = false;
        if (DEFAULT_REPAIR == op.mode || EXT_REPAIR == op.mode) {
            bool reboot = query_reboot_now(reboot_query, reboot_confirms,reboot_planned);
            if (reboot) {
                system("C:\\Windows\\System32\\shutdown.exe /r /t 0");
                break;
            }
        }
        add = NULL;
        if (verbose_current) {
            time(&repair_end_time);
            long long diff_sec = repair_end_time - repair_start_time;
            create_time_str(time_buffer, MAX_LOCALIZED_STRING_SIZE, diff_sec);
            swprintf(repair_time_str, MAX_LOCALIZED_STRING_SIZE, repair_time_fmt, time_buffer);
            add = repair_time_str;
        }
        show_repair_finish_message(add);
    }
    return 0;
}

void create_time_str(wchar_t* buffer, size_t size, long long sec) {
    ZeroMemory(buffer, size);
    wchar_t unit_buffer[MAX_LOCALIZED_STRING_SIZE] = {};
    if (sec >= 86400) {
        long long days = sec / 86400;
        swprintf(unit_buffer, sizeof(unit_buffer), L" %lld %s", days, days_str);
        wcscat_s(buffer, size, unit_buffer);
        sec %= 86400;
    }
    if (sec >= 3600) {
        long long hours = sec / 3600;
        swprintf(unit_buffer, sizeof(unit_buffer), L" %lld %s", hours, hours_str);
        wcscat_s(buffer, size, unit_buffer);
        sec %= 3600;
    }
    if (sec >= 60) {
        long long minutes = sec / 60;
        swprintf(unit_buffer, sizeof(unit_buffer), L" %lld %s", minutes, minutes_str);
        wcscat_s(buffer, size, unit_buffer);
        sec %= 60;
    }
    if (sec > 0) {
        swprintf(unit_buffer, sizeof(unit_buffer), L" %lld %s", sec, seconds_str);
        wcscat_s(buffer, size, unit_buffer);
    }
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    if (running) {
        show_message(abort_msg);
        stop = true;
        return TRUE;
    }
    return FALSE;
}


int start_process(wstring command, PROCESS_INFORMATION * proc_info, HANDLE * subprocess_in_write_ptr, HANDLE * subprocess_out_read_ptr) {
#ifdef DEBUG
    std::wcout << std::endl << L"Command: " << command << endl;
#endif // DEBUG

    // handles for communication with the created process
    HANDLE subprocess_in_read = NULL, subprocess_out_write = NULL;

    // Security attr for making the handles inheritable
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // creating pipes
    if (!CreatePipe(subprocess_out_read_ptr, &subprocess_out_write, &saAttr, 0)) {
        return -10;
    }
    if (!SetHandleInformation(*subprocess_out_read_ptr, HANDLE_FLAG_INHERIT, 0)) {
        return -11;
    }
    if (!CreatePipe(&subprocess_in_read, subprocess_in_write_ptr, &saAttr, 0)) {
        return -20;
    }
    if (!SetHandleInformation(*subprocess_in_write_ptr, HANDLE_FLAG_INHERIT, 0)) {
        return -21;
    }

    // structures for creating the process
    STARTUPINFO start_info;
    BOOL success = FALSE;

    ZeroMemory(proc_info, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&start_info, sizeof(STARTUPINFO));

    // setting the previously created pipes as stdin and stdout (and stderr)
    start_info.cb = sizeof(STARTUPINFO);
    start_info.hStdError = subprocess_out_write;
    start_info.hStdOutput = subprocess_out_write;
    start_info.hStdInput = subprocess_in_read;
    start_info.dwFlags |= STARTF_USESTDHANDLES;

    success = CreateProcess(NULL,
        (LPWSTR)command.c_str(),
        NULL,
        NULL,
        TRUE,
        HIGH_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP,
        NULL,
        NULL,
        &start_info,
        proc_info);

    if (!success) {
        return -1;
    }
    else
    {
        CloseHandle(subprocess_out_write);
        CloseHandle(subprocess_in_read);
    }

    return 0;
}

void wait_for_process(PROCESS_INFORMATION* proc_info) {
    WaitForSingleObject(proc_info->hProcess, INFINITE);
    CloseHandle(proc_info->hProcess);
    CloseHandle(proc_info->hThread);
}

void print_help() {
    wstring batch_path;
    wchar_t wrt_path[MAX_PATH];
    if (0 == GetModuleFileName(NULL, wrt_path, MAX_PATH)) {
        batch_path = L"NOT FOUND";// unlikely but just in case
    }
    else if (S_OK == PathCchRemoveFileSpec(wrt_path, MAX_PATH)) {
        if (S_OK == PathCchRemoveFileSpec(wrt_path, MAX_PATH)) {
            wchar_t batch_dir[] = L"Batch scripts";
            if (S_OK == PathCchAppend(wrt_path, MAX_PATH, batch_dir)) {
                batch_path = wstring(wrt_path);
            }
            else {
                batch_path = L"NOT FOUND";// unlikely but just in case
            }
        }
        else {
            batch_path = L"NOT FOUND";// unlikely but just in case
        }
    }
    else {
        batch_path = L"NOT FOUND";// unlikely but just in case
    }
    wchar_t help_text[MAX_LOCALIZED_STRING_SIZE];
    swprintf(help_text, MAX_LOCALIZED_STRING_SIZE, help_text_fmt, batch_path.c_str());
    show_help(help_text);
}