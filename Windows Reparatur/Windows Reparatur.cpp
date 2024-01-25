#include <iostream>
#include <Windows.h>
#include <string>
#include "resource.h"
#include "localization.h"

using namespace std;

int total;
int counter;
void exec(string command);
void printWarning(wstring warn);

int main()
{

    load_localized_strings();
    string standardReparatur[] = {
        "defrag C: /O /H",
        "sfc /scannow",
        "\"\"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe\" -Scan -ScanType 1\"",
        "sfc /scannow",
        "dism /online /cleanup-image /CheckHealth",
        "Dism /Online /Cleanup-Image /ScanHealth",
        "Dism /Online /Cleanup-Image /RestoreHealth",
        "chkdsk C: /scan /perf /i",
        "sfc /scannow",
        "defrag C: /O /H"
    };
    string erweiterteReparatur[] = {
        "defrag C: /O /H",
        "sfc /scannow",
        "\"\"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe\" -Scan -ScanType 2\"",
        "sfc /scannow",
        "dism /online /cleanup-image /CheckHealth",
        "Dism /Online /Cleanup-Image /ScanHealth",
        "Dism /Online /Cleanup-Image /RestoreHealth",
        "sfc /scannow",
        "defrag C: /O /H",
        "chkdsk C: /scan /perf /f /r /x /b < bestaetigung.txt"
    };

    string zusatzReperatur[] = {
        "defrag /c /o /h /m",
        "sfc /scannow",
        "chkdsk C: /f /x /spotfix /sdcleanup < bestaetigung.txt"
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

    char input[3];
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

        std::cin.get(input, 3);
        if (input[1] == 0) {
            std::cin.ignore(INT16_MAX, '\n');
            auswahl = input[0] - 48;
        }
        
        if (auswahl != 1 && auswahl != 2 && auswahl != 3) {
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
        
        wchar_t started[MAX_LOCALIZED_STRING_SIZE];
        swprintf(started, MAX_LOCALIZED_STRING_SIZE, progress_started_fmt, 1, total);
        wcout << std::endl << L" " << started;
        for (int i = 0; i < total; i++)
        {
            if (auswahl == 1)
            {
                exec(standardReparatur[i]);
            }
            else if (auswahl == 2)
            {
                exec(erweiterteReparatur[i]);
            }
            else
            {
                exec(zusatzReperatur[i]);
            }
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
        wchar_t timing[MAX_LOCALIZED_STRING_SIZE];
        swprintf(timing, MAX_LOCALIZED_STRING_SIZE, exec_time_fmt, clock() - start_time);
        wcout << L" " << timing << endl;
#endif // DEBUG
        wcout << std::endl << std::endl << std::endl;
    }
    return 0;
}

void exec(string command) {
    string  line = command;// +" > process_" + to_string(counter) + ".txt 2>&1"; // redirecting to output to files causes Windows Defender to detect this as Virus
#ifdef DEBUG
    cout << std::endl << "Command: " << line << endl;
#endif // DEBUG
    system(line.c_str());

    wchar_t done[MAX_LOCALIZED_STRING_SIZE];
    swprintf(done, MAX_LOCALIZED_STRING_SIZE, progress_done_fmt, counter++,total);
    wcout << L"\r " << done;
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