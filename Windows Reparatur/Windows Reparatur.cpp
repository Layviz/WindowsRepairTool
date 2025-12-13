#include <iostream>
#include <Windows.h>
#include <PathCch.h>
#include <string>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <chrono>
#include "resource.h"
#include "localization.h"
#include "wrt_types.h"

#define WINTERMBOX_IMPL
#include "../WinTermbox.h"

using namespace std;


int total;
int counter;
int start_process(wstring command, PROCESS_INFORMATION* proc_info,HANDLE * input,HANDLE * output);
void wait_for_process(PROCESS_INFORMATION* proc_info);
void create_time_str(std::wstring& buffer, long long sec);
void printWarning(wstring warn);
void print_help();
bool read_user_intput(OPERATION* op);
bool verbose = false;   //verbose for all runs
bool stop = false, running = false;
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
int get_console_height();
void dism_progress_bar_fix(char* buffer, DWORD size);
void sfc_output_fix(char* buffer, DWORD size);
int setup_virt_console();
void show_logo();

int main()
{
    BOOL virtual_console = setup_virt_console();
    if (!virtual_console) {
        std::wcout << L" Error on setting up virtual Console!" << endl;
    }
    else {
        show_logo();
    }

    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
        // ignore this, not much to do about it
    }

    load_localized_strings();
    wstring simpleRepair[] = {
        L"defrag C: /o /h",
        L"sfc /scannow",
        L"mpcmdrun.exe -Scan -ScanType 1",
        L"chkdsk C: /scan /perf /i",
        L"dism /online /cleanup-image /CheckHealth",
        L"dism /Online /Cleanup-Image /ScanHealth",
        L"dism /Online /Cleanup-Image /RestoreHealth",
        L"chkdsk C: /scan /perf",
        L"sfc /scannow",
        L"cleanmgr /sagerun:151",
        L"defrag C: /o /h"
    };
    wstring defaultRepair[] = {
        L"defrag C: /o /h",
        L"sfc /scannow",
        L"mpcmdrun.exe -Scan -ScanType 2",
        L"chkdsk C: /scan /perf",
        L"dism /online /cleanup-image /CheckHealth",
        L"dism /Online /Cleanup-Image /ScanHealth",
        L"dism /Online /Cleanup-Image /RestoreHealth",
        L"sfc /scannow",
        L"defrag /c /o /h /m",
        L"cleanmgr /sagerun:152",
        L"chkdsk C: /scan /perf /f /r /x /b"
    };

    wstring extendedRepair[] = {
        L"cleanmgr /sagerun:153",
        L"defrag C: /o /h",
        L"chkdsk C: /scan /perf /i",
        L"sfc /scannow",
        L"chkdsk C: /f /x /spotfix /sdcleanup"
    };

#ifdef DEBUG
    std::wcout << L"Loaded Strings" << endl;
    std::wcout << mutex_warn << endl;
    std::wcout << pending_query << endl;
    std::wcout << pending_option1 << endl;
    std::wcout << pending_option2 << endl;
    std::wcout << pending_option3 << endl;
    std::wcout << startup_warn << endl;
    std::wcout << mode_query << endl;
    std::wcout << mode_option1 << endl;
    std::wcout << mode_option2 << endl;
    std::wcout << mode_option3 << endl;
    std::wcout << mode_cancel << endl;
    std::wcout << in_progress_note << endl;
    std::wcout << progress_started_fmt << endl;
    std::wcout << progress_done_fmt << endl;
    std::wcout << reboot_query << endl;
    std::wcout << reboot_confirms << endl;
    std::wcout << reboot_planned << endl;
    std::wcout << exec_time_fmt << endl;
#endif // DEBUG

    HANDLE mutex = CreateMutex(NULL, false, L"Local\\WRT");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        wcerr << mutex_warn << endl;
        cin.get();
        return -1;
    }
    std::wcout << endl;


    char input[4] = {};
    int auswahl = 0;
    // test for pending.xml
    GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml");
    if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml") && GetLastError() != ERROR_FILE_NOT_FOUND) {
     // fragen ob man trotzdem reparieren will
        std::wcout << L" " << pending_query << endl << endl;
        std::wcout << L" " << pending_option1 << endl;
        std::wcout << L" " << pending_option2 << endl;

        do {
        std::wcout << std::endl << L" " << mode_cancel << std::endl << L" ";
            std::cin.get(input, 3);
            if (input[1] == 0) {
                std::cin.ignore(INT16_MAX, '\n');
                auswahl = input[0] - 48;
            }
            if ('h' == input[0]) {
                std::wcout << endl << pending_help_text;
            
            }

        } while ('h' == input[0]);
        int height;
        switch (auswahl)
        {
        case 1:
            system("shutdown /r /t 0");
            return 0;
        case 2:
            //disabled for now 
            /*
             height = get_console_height();
            for (int i = 0; i < height; i++) {
                wcout << endl;
            }
            */
            std::wcout << endl;
            break;
        default:
            return 0;
        }
    }

    std::wcout << endl << endl;

    if (!SetConsoleCtrlHandler( CtrlHandler, TRUE)) {
#ifdef DEBUG
        printf("handler not registred\n");
#endif
    }

    if (!DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND)) {
#ifdef DEBUG
        printf("close button not removed\n");
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


    wstring header_warning = startup_warn;
    printWarning(header_warning);

    wstring repair_time_str;
    repair_time_str.resize(MAX_LOCALIZED_STRING_SIZE);
    wstring start_time_str;
    start_time_str.resize(MAX_LOCALIZED_STRING_SIZE);
    wstring end_time_str;
    end_time_str.resize(MAX_LOCALIZED_STRING_SIZE);
    wstring time_buffer;
    time_buffer.resize(MAX_LOCALIZED_STRING_SIZE);
    wstring done;
    done.resize(MAX_LOCALIZED_STRING_SIZE);
    wchar_t* add;

    std::wcout << endl << endl << endl;
    /*-------------
    *             *
    *  MAIN LOOP  *
    *             *
    *-------------*/
    while (!stop) {
        OPERATION op = {};
        bool r;

        bool verbose_current = false; //verbose for the next run

        std::wcout << L" " << mode_query << std::endl << std::endl;
        std::wcout << L" " << mode_option1 << std::endl;
        std::wcout << L" " << mode_option2 << std::endl;
        std::wcout << L" " << mode_option3 << std::endl;
        std::wcout << std::endl << L" " << mode_cancel << std::endl << L" ";

        r = read_user_intput(&op);
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
            break;
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
                std::wcout << endl << L" " << in_progress_note << endl;
            }
            time_t process_start_time,process_end_time;
            time(&process_start_time);
            PROCESS_INFORMATION proc_info;
            HANDLE input = NULL, output = NULL;
            int rv = 0;
            wchar_t timestr[16];
            wstring command;
            if (SIMPLE_REPAIR == op.mode)
            {
                rv = start_process(simpleRepair[i],&proc_info,&input,&output);
                command = simpleRepair[i];
            }
            else if (DEFAULT_REPAIR == op.mode)
            {
                rv = start_process(defaultRepair[i], &proc_info, &input, &output);
                command = defaultRepair[i];
            }
            else if (EXT_REPAIR == op.mode)
            {
                rv = start_process(extendedRepair[i], &proc_info, &input, &output);
                command = extendedRepair[i];
            }
            if (rv <= -10) {
                std::wcout << endl << L"ERROR on creating pipes for subprocess" << endl;
            }
            else if (rv < 0) {
                std::wcout << endl << L"ERROR on creating subprocess" << endl;
                wchar_t error_buffer[1024];
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), error_buffer, 1024, NULL);
                std::wcout << L"Error: " << error_buffer<<endl;
            }
            else if (0 == rv) {
                _wstrtime_s(timestr);
                swprintf(&done[0], MAX_LOCALIZED_STRING_SIZE, progress_started_fmt, counter, total);
                wtb_clear_current_line();
                std::wcout << L" " << done;
                if (verbose_current) {
                    swprintf(&start_time_str[0], MAX_LOCALIZED_STRING_SIZE, process_start_time_fmt, timestr);
                    std::wcout << L" " << start_time_str;
                }
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
                std::wcout << endl << endl << endl;
                do {
                    success = ReadFile(output, buffer, 512, &read, NULL);
                    if (success) {
                        if (command.substr(0,4)==L"dism") {
                            dism_progress_bar_fix(buffer, read);
                        }
                        else if (command.substr(0, 3) == L"sfc") {
                            sfc_output_fix(buffer, read);
                        }
                        std::string narrow(buffer, read);
                        std::wstring wide = std::wstring(narrow.begin(), narrow.end());
                        std::wcout << wide;

                    }
                    else {
#ifdef DEBUG
                        std::wcout << "error on read" << endl;
#endif
                    }
                    ZeroMemory(buffer, sizeof(buffer));
                } while (success);
                std::wcout << endl << endl;
            }
            if(input)
                CloseHandle(input);
            if(output)
                CloseHandle(output);
            wait_for_process(&proc_info);
            time(&process_end_time);
            _wstrtime_s(timestr);
            swprintf(&done[0], MAX_LOCALIZED_STRING_SIZE, progress_done_fmt, counter++, total);
			wtb_clear_current_line();
            std::wcout << L" " << done;
            if (verbose_current) {
                long long diff_sec = process_end_time - process_start_time;
                create_time_str(time_buffer, diff_sec);
                swprintf(&end_time_str[0], MAX_LOCALIZED_STRING_SIZE, process_end_time_fmt, timestr, time_buffer.c_str());
                wprintf(L" %s\n\n", end_time_str.c_str());
                
            }
            if (stop)
                break;
        }
        time(&repair_end_time);
        running = false;
        if (verbose_current) {
            long long diff_sec = repair_end_time - repair_start_time;
            create_time_str(time_buffer, diff_sec);
            swprintf(&repair_time_str[0], MAX_LOCALIZED_STRING_SIZE, repair_time_fmt, time_buffer.c_str());
            std::wcout << L" " << repair_time_str << endl;
        }
        if (DEFAULT_REPAIR == op.mode || EXT_REPAIR == op.mode) {
            std::wcout << std::endl<<std::endl<< L" " << reboot_query;
            std::cin.get(input, 3);
            std::cin.ignore(INT16_MAX, '\n');
            if (0==input[1]){ 
                int reboot = 0;
                size_t confirm_len = wcslen(reboot_confirms);
                for (int i = 0; i < confirm_len; i++) {
                    if (input[0] == reboot_confirms[i]) {
                        reboot = 1;
                        break;
                    }
                }
                if (reboot) {
                    system("C:\\Windows\\System32\\shutdown.exe /r /t 0");
                    return 0;
                }
            }
            std::wcout << std::endl << L" " << reboot_planned << endl;
        }
        std::wcout << std::endl << std::endl << std::endl;
    }
    return 0;
}

void create_time_str(std::wstring& buffer, long long sec) {
    buffer.clear();

    if (sec >= 86400) {
        long long days = sec / 86400;
        buffer += L" " + std::to_wstring(days) + L" " + days_str;
        sec %= 86400;
    }
    if (sec >= 3600) {
        long long hours = sec / 3600;
        buffer += L" " + std::to_wstring(hours) + L" " + hours_str;
        sec %= 3600;
    }
    if (sec >= 60) {
        long long minutes = sec / 60;
        buffer += L" " + std::to_wstring(minutes) + L" " + minutes_str;
        sec %= 60;
    }
    if (sec >= 0) {
        buffer += L" " + std::to_wstring(sec) + L" " + seconds_str;
    }
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    if (running) {
        std::wcout << endl << L" " << abort_msg << endl;
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

void printWarning(wstring warn) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    wstring header_centralline = L"|     " + warn + L"     |";
    wstring header_topline = L"+" + wstring(header_centralline.size() - 2, '-') + L"+";
    wstring header_padline = L"|" + wstring(header_centralline.size() - 2, ' ') + L"|";

    size_t startpoint = (columns - header_centralline.size()) / 2;

    std::wcout << L"\x1b[93m";
    std::wcout << wstring(startpoint, ' ') << header_topline << endl;
    std::wcout << wstring(startpoint, ' ') << header_padline << endl;
    std::wcout << wstring(startpoint, ' ') << header_centralline << endl;
    std::wcout << wstring(startpoint, ' ') << header_padline << endl;
    std::wcout << wstring(startpoint, ' ') << header_topline << endl;
    std::wcout << L"\x1b[0m";
}

int get_console_height() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top;
}

bool read_user_intput(OPERATION* op) {
    char input[4] = {};
    std::cin.get(input, 3);
    if (cin.fail()) {
        cin.clear();
    }
    std::cin.ignore(INT16_MAX, '\n');
    if (cin.fail()) {
        cin.clear();
    }
    switch (input[0]) {
    case '1':
        op->mode = SIMPLE_REPAIR;
        break;
    case '2':
        op->mode = DEFAULT_REPAIR;
        break;
    case '3':
        op->mode = EXT_REPAIR;
        break;
    case 'h':
    case '?':
        op->mode = HELP;
        break;
    case '+':
        op->mode = VERBOSE_ON;
        break;
    case '-':
        op->mode = VERBOSE_OFF;
        break;
    default:
        op->mode = NO_OP;
        break;
    }
    switch (input[1]) {
    case '+':
        if (op->mode<SIMPLE_REPAIR || op->mode>EXT_REPAIR) {
            return false;
        }
        op->verbose = VERBOSE;
        break;
    case '-':
        if (op->mode<SIMPLE_REPAIR || op->mode>EXT_REPAIR) {
            return false;
        }
        op->verbose = SILENT;
        break;
    case 0:
        op->verbose = DEFAULT;
        break;
    default:
        return false;
    }
    if (input[2] != 0) {
        return false;
    }

    return true;
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
    std::wcout << endl << endl << help_text << L" ";

}

void dism_progress_bar_fix(char* buffer, DWORD size) {
    char* pos;
    while (pos = strstr(buffer, "] \r\n")) {
        if(!strstr(buffer,"100.0%"))
            *(pos + 3) = '\r';
        buffer = pos + 4;
    }
}

void sfc_output_fix(char* buffer, DWORD size) {
    char* read = buffer, * write = buffer;
    for (DWORD i = 0; i < size; i++) {
        read = buffer + i;
        if (*read) {
            *write = *read;
            write++;
        }
    }
    if (write < buffer + size) {
        *write = 0;
    }
}

int setup_virt_console() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return 0;

    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 18;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Cascadia Code");
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);

    return wtb_init();
}

bool gay_mode() {
    using namespace std::chrono;
    zoned_time local_time{ current_zone(), system_clock::now() };
    year_month_day datum{ floor<days>(local_time.get_local_time()) };
    return datum.month() == June;
}

/*

"                                               __    __"
"__                __  ______    ___________   ///    \\\\\\"
"\\\\\\      __      /// ||||||\\\\  |≣≣≣≣≣≣≣≣| (\\\\\\____///"
" \\\\\\    //\\\\    ///  |||__)||)      |||       ◥≣≣≣▩◤"
"  \\\\\\  ///\\\\\\  ///   ||||||//       |||        ////‾"
"   \\\\\\////\\\\\\\\///    |||  \\\\\\       |||       ////"
"    \\\\\\//  \\\\///     |||   \\\\\\      |||      ////"
"     \\\\/    \\//      |||    \\\\\\     |||     ////"
"      ‾      ‾        ‾      ‾‾      ‾      \\≣/"

*/

void show_logo() {
    DWORD wait = 70, final_wait = 1200;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;

	const int logo_width = 58;

	int left_padding = (columns - logo_width) / 2;
    
    _setmode(_fileno(stdout), _O_U16TEXT);

	wtb_clear();
    wtb_set_cursor_visible(WTB_CURSOR_VISIBILITY_OFF);
    
    if (gay_mode()) wtb_set_custom_foreground_color(239, 57, 52);;
	wprintf(wstring(left_padding,L' ').c_str());
    wprintf(L"                                                __   __\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    wprintf(L" __                __  ______    ___________   ///   \\\\\\\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    wprintf(L" \\\\\\      __      /// ||||||\\\\  |\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263|  \\\\\\___///\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    if (gay_mode()) wtb_set_custom_foreground_color(244, 121, 51);
    wprintf(L"  \\\\\\    //\\\\    ///  |||__)||)      |||        \u25e5\u25a9\u2263\u2263\u2263\u25a9\u25e4\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    if (gay_mode()) wtb_set_custom_foreground_color(244, 235, 59);
    wprintf(L"   \\\\\\  ///\\\\\\  ///   ||||||//       |||        ////\u203e\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    if (gay_mode()) wtb_set_custom_foreground_color(95, 187, 70);
    wprintf(L"    \\\\\\////\\\\\\\\///    |||  \\\\\\       |||       ////\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    if (gay_mode()) wtb_set_color(WTB_COLOR_BLUE, WTB_COLOR_DEFAULT);
    wprintf(L"     \\\\\\//  \\\\///     |||   \\\\\\      |||      ////\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    if (gay_mode()) wtb_set_custom_foreground_color(198, 42, 138);
    wprintf(L"      \\\\/    \\//      |||    \\\\\\     |||     ////\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(wstring(left_padding, L' ').c_str());
    wprintf(L"       \u203e      \u203e        \u203e      \u203e\u203e      \u203e      \\\u2263/\n");
    fflush(stdout);
    Sleep(final_wait);


    for (int i = 0; i < 11; i++)
    {
        wtb_clear_line(i);
        Sleep(wait);
    }
    wtb_set_color(WTB_COLOR_DEFAULT, WTB_COLOR_DEFAULT);
	wtb_clear();
	wtb_set_cursor_visible(WTB_CURSOR_VISIBILITY_ON);
}