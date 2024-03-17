#include <iostream>
#include <Windows.h>
#include <string>
#include "resource.h"
#include "localization.h"

using namespace std;

int total;
int counter;
int start_process(wstring command, PROCESS_INFORMATION* proc_info,HANDLE * input,HANDLE * output);
void wait_for_process(PROCESS_INFORMATION* proc_info);
void printWarning(wstring warn);
wstring blank_line(80, L' '); //80 spaces should be enough to overwrite everthing
bool verbose = false;   //verbose for all runs
bool verbose_current = false; //verbose for the next run

int main()
{

    load_localized_strings();
    wstring standardReparatur[] = {
        L"defrag C: /O /H",
        L"sfc /scannow",
        L"\"\"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe\" -Scan -ScanType 1\"",
        L"sfc /scannow",
        L"dism /online /cleanup-image /CheckHealth",
        L"Dism /Online /Cleanup-Image /ScanHealth",
        L"Dism /Online /Cleanup-Image /RestoreHealth",
        L"chkdsk C: /scan /perf /i",
        L"sfc /scannow",
        L"defrag C: /O /H"
    };
    wstring erweiterteReparatur[] = {
        L"defrag C: /O /H",
        L"sfc /scannow",
        L"\"\"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe\" -Scan -ScanType 2\"",
        L"sfc /scannow",
        L"dism /online /cleanup-image /CheckHealth",
        L"Dism /Online /Cleanup-Image /ScanHealth",
        L"Dism /Online /Cleanup-Image /RestoreHealth",
        L"sfc /scannow",
        L"defrag C: /O /H",
        L"chkdsk C: /scan /perf /f /r /x /b"
    };

    wstring zusatzReperatur[] = {
        L"defrag /c /o /h /m",
        L"sfc /scannow",
        L"chkdsk C: /f /x /spotfix /sdcleanup"
    };

#ifdef DEBUG
    wcout << L"Loaded Strings" << endl;
    wcout << mutex_warn << endl;
    wcout << pending_query << endl;
    wcout << pending_option1 << endl;
    wcout << pending_option2 << endl;
    wcout << pending_option3 << endl;
    wcout << startup_warn << endl;
    wcout << mode_query << endl;
    wcout << mode_option1 << endl;
    wcout << mode_option2 << endl;
    wcout << mode_option3 << endl;
    wcout << mode_cancel << endl;
    wcout << in_progress_note << endl;
    wcout << progress_started_fmt << endl;
    wcout << progress_done_fmt << endl;
    wcout << reboot_query << endl;
    wcout << reboot_confirms << endl;
    wcout << reboot_planned << endl;
    wcout << exec_time_fmt << endl;
#endif // DEBUG

    HANDLE mutex = CreateMutex(NULL, false, L"Local\\WRT");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        wcerr << mutex_warn << endl;
        cin.get();
        return -1;
    }
    wcout << endl;


    char input[4];
    int auswahl = 0;
    // test for pending.xml
    GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml");
    if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml") && GetLastError() != ERROR_FILE_NOT_FOUND) {
        // fragen ob man trotzdem reparieren will
        wcout << L" " << pending_query << endl;
        wcout << L" " << pending_option1 << endl;
        wcout << L" " << pending_option2 << endl;
        wcout << L" " << pending_option3 << endl;

        std::cin.get(input, 3);
        if (input[1] == 0) {
            std::cin.ignore(INT16_MAX, '\n');
            auswahl = input[0] - 48;
        }

        switch (auswahl)
        {
        case 1:
            system("shutdown /r /t 0");
            break;
        case 3:
            break;
        case 2:
        default:
            return 0;
        }
    }

    if (SetConsoleCtrlHandler( NULL, TRUE)) {
        ;
    }
    else {
#ifdef DEBUG
        printf("handler not registred\n");
#endif
    }

    if (DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND)) {
        ;
    }
    else {
#ifdef DEBUG
        printf("close button not removed\n");
#endif
    }


    wstring header_warning = startup_warn;
    printWarning(header_warning);

    wcout << endl << endl << endl;
    while (true) {
        int auswahl = 0;
        wcout << L" " << mode_query << std::endl << std::endl;
        wcout << L" " << mode_option1 << std::endl;
        wcout << L" " << mode_option2 << std::endl;
        wcout << L" " << mode_option3 << std::endl;
        wcout << std::endl << L" " << mode_cancel << std::endl << " ";

        std::cin.get(input, 4);
        if ((input[1] == 0) || (input[1] == '+' && input[2] == 0)) {
            std::cin.ignore(INT16_MAX, '\n');
            if (input[0] == '+') {
                verbose = true;
                wcout << std::endl << std::endl << std::endl;
                continue;
            }
            else if (input[0] == '-') {
                verbose = false;
                wcout << std::endl << std::endl << std::endl;
                continue;
            }
            auswahl = input[0] - 48;
            if (input[1] == '+') {
                verbose_current = true;
            }
            
        }
        if (verbose)
            verbose_current = true;

        if ((auswahl < 1 || auswahl > 3) && auswahl != -5 && auswahl != -3) {
#ifdef DEBUG
            if (auswahl>4)
#endif // DEBUG

            break;
        }

#ifdef DEBUG
        const clock_t start_time = clock();
#endif // DEBUG

        wcout << endl << L" " << in_progress_note << endl;

        if (auswahl == 1) {
            total = sizeof(standardReparatur) / sizeof(string);
        }
        else if (auswahl == 2) {
            total = sizeof(erweiterteReparatur) / sizeof(string);
        }
        else {
            total = sizeof(zusatzReperatur) / sizeof(string);
        }
        counter = 1;
        
        for (int i = 0; i < total; i++)
        {
            PROCESS_INFORMATION proc_info;
            HANDLE input = NULL, output = NULL;
            if (auswahl == 1)
            {
                start_process(standardReparatur[i],&proc_info,&input,&output);
            }
            else if (auswahl == 2)
            {
                start_process(erweiterteReparatur[i], &proc_info, &input, &output);
            }
            else if (auswahl == 3)
            {
                start_process(zusatzReperatur[i], &proc_info, &input, &output);
            }

            if (i == total-1) {
                wchar_t confirmation[] = {reboot_confirms[0],'\r','\n','\0'};
                DWORD written = 0;
                //wcout << L"Confirmation: " << confirmation << endl;
                WriteFile(input, confirmation, sizeof(confirmation), &written, NULL);
                //wcout << L"bytes written " << written << " of " << sizeof(confirmation) << endl;
            }
            if (verbose_current && output) {
                char buffer[512] = {};
                DWORD read = 0;
                BOOL success;
                wcout << endl << endl << endl;
                do {
                    success = ReadFile(output, buffer, 512, &read, NULL);
                    if (success) {
                        if (read < 512) {
                            cout << " ";
                        }
                        cout << buffer;
                    }
                    else {
#ifdef DEBUG
                        wcout << "error on read" << endl;
#endif
                    }
                    memset(buffer, 0, 512);
                } while (success);
                wcout << endl << endl;
            }
            if(input)
                CloseHandle(input);
            if(output)
                CloseHandle(output);
            wait_for_process(&proc_info);
            wchar_t done[MAX_LOCALIZED_STRING_SIZE];
            swprintf(done, MAX_LOCALIZED_STRING_SIZE, progress_done_fmt, counter++, total);
            wcout << L"\r " << blank_line << L"\r " << done;
        }
        if (2 == auswahl || 3 == auswahl) {
            wcout << std::endl<<std::endl<< L" " << reboot_query;
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
                    system("shutdown /r /t 0");
                    break;
                }
            }
            wcout << std::endl << L" " << reboot_planned << endl;
        }
#ifdef DEBUG
        clock_t time_diff = clock() - start_time;
        wchar_t timing[MAX_LOCALIZED_STRING_SIZE];
        swprintf(timing, MAX_LOCALIZED_STRING_SIZE, exec_time_fmt, time_diff);
        wcout << L"\n " << timing << time_diff << endl;
#endif // DEBUG
        wcout << std::endl << std::endl << std::endl;
        verbose_current = false;
    }
    return 0;
}

int start_process(wstring command, PROCESS_INFORMATION * proc_info, HANDLE * subprocess_in_write_ptr, HANDLE * subprocess_out_read_ptr) {
#ifdef DEBUG
    wcout << std::endl << L"Command: " << command << endl;
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
        HIGH_PRIORITY_CLASS,
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

    wchar_t done[MAX_LOCALIZED_STRING_SIZE];
    swprintf(done, MAX_LOCALIZED_STRING_SIZE, progress_started_fmt, counter,total);
    wcout << L"\r " << blank_line << L"\r " << done;
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

    wcout << wstring(startpoint, ' ') << header_topline << endl;
    wcout << wstring(startpoint, ' ') << header_padline << endl;
    wcout << wstring(startpoint, ' ') << header_centralline << endl;
    wcout << wstring(startpoint, ' ') << header_padline << endl;
    wcout << wstring(startpoint, ' ') << header_topline << endl;
}