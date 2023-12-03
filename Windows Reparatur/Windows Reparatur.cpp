#include <iostream>
#include <Windows.h>
#include <string>
#include "resource.h"

using namespace std;

int total;
int counter;
void exec(string command);
void printWarning(string warn);

// localized strings
#define MAX_LOCALIZED_STRING_SIZE 512
wchar_t mutex_warn[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t pending_query[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t pending_option1[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t pending_option2[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t pending_option3[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t startup_warn[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t mode_query[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t mode_option1[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t mode_option2[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t mode_option3[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t mode_cancel[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t in_progress_note[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t progress_started_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t progress_done_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t reboot_query[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t reboot_confirms[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t reboot_planned[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t exec_time_fmt[MAX_LOCALIZED_STRING_SIZE] = {};

void load_stings(HINSTANCE source) {
    LoadString(source, MUTEX_WARN, mutex_warn, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PENDING_RESTART_QUERY, pending_query, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PENDING_RESTART_OPTION1, pending_option1, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PENDING_RESTART_OPTION2, pending_option2, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PENDING_RESTART_OPTION3, pending_option3, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, STARTUP_WARNING, startup_warn, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, MODE_QUERY, mode_query, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, MODE_OPTION1, mode_option1, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, MODE_OPTION2, mode_option2, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, MODE_OPTION3, mode_option3, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, MODE_CANCEL_INFO, mode_cancel, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, IN_PROGRESS_NOTE, in_progress_note, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PROGRESS_STARTED_FORMAT_STRING, progress_started_fmt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PROGESS_DONE_FORMAT_STRING, progress_done_fmt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, REBOOT_QUERY, reboot_query, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, REBOOT_CONFIRMS, reboot_confirms, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, REBOOT_PLANNED, reboot_planned, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, EXECUTION_TIME_FORMAT_STRING, exec_time_fmt, MAX_LOCALIZED_STRING_SIZE);
}

int main()
{
    //SetConsoleOutputCP(65001);

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

    wchar_t locale_name[3] = {};
    wchar_t dll_name[7] = {};

    GetUserDefaultLocaleName(locale_name, 3);
    wcout << locale_name << endl;
    swprintf(dll_name,7,L"%s.dll",locale_name);
    wcout << dll_name << endl;
    // Try loading a lib with the local name
    HMODULE locale_source = LoadLibrary(dll_name);
    if (nullptr == locale_source) {
        // if no locale lib exists use the default strings
        locale_source = GetModuleHandle(nullptr);
    }
    
    load_stings(locale_source);

#ifdef DEBUG

    wchar_t test_string[MAX_LOCALIZED_STRING_SIZE] = {};
    LoadString(locale_source, TEST_STRING, test_string, MAX_LOCALIZED_STRING_SIZE);

    wchar_t test[] = L"äüö";
    wcout << (int)test[0] << L" " << (int)test_string[0] << endl;
    wcout << (int)test[1] << L" " << (int)test_string[1] << endl;
    wcout << (int)test[2] << L" " << (int)test_string[2] << endl;
    int test_conv = (int) test;
    wcout << L"ä as int: " << test_conv << endl;
    wcout << "In wchar " << (int) mutex_warn[32] << endl;
    wcout << L"Umlaute: \xc300\u00f6\u00fc" << endl ;
    wcout << L"Loaded Strings" << endl;
    wcout << mutex_warn << endl;
    wcout << pending_query << endl;
    wcout << pending_option1 << endl;
    wcout << pending_option2 << endl;
    wcout << pending_option3 << endl;
    wcout << startup_warn << endl;
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
        cerr << "Das Programm wird bereits ausgeführt und kann nur einmal aufgerufen werden." << endl;
        cin.get();
        return -1;
    }
    cout << endl;

    char input[3];
    // test for pending.xml
    GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml");
    if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(L"C:\\Windows\\WinSxS\\pending.xml") && GetLastError() != ERROR_FILE_NOT_FOUND) {
        // fragen ob man trotzdem reparieren will
        cout << " Es steht noch ein Systemneustart aus. Die Reparatur kann ohne Neustart nicht vollständig abgeschlossen werden." << endl;
        cout << " 1. Neustart " << endl;
        cout << " 2. Beenden" << endl;
        cout << " 3. Trotzdem ausführen" << endl;
    }


    string header_warning = "Es wird empfohlen nebenbei keine aufwendigen Prozesse laufen zu lassen.";
    printWarning(header_warning);

    cout << endl << endl << endl;
    while (true) {
        int auswahl = 0;
        std::cout << " System-Reparaturmodus wählen:" << std::endl << std::endl;
        std::cout << " 1. Einfache Reparatur    (Dauert wenige Minuten, kein Neustart erforderlich.)" << std::endl;
        std::cout << " 2. Standard Reparatur    (Kann mehrere Stunden dauern, Neustart erforderlich.)" << std::endl;
        std::cout << " 3. Erweiterte Reparatur  (Empfohlen nach der Standard Reparatur, Neustart erforderlich.)" << std::endl;
        std::cout << std::endl << " Beliebige Eingabe tätigen, um das Programm zu beenden." << std::endl << " ";

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

        cout << endl << " Reparatur wird durchgeführt. Bitte nicht das Programm beenden oder den Computer ausschalten." << endl;

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
        
        std::cout << std::endl << " Prozess " << 1 << " von " << total << " gestartet";
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
            std::cout << std::endl<<std::endl<< " Um die Reparatur abzuschließen ist ein Systemneustart erforderlich, Jetzt Neustarten? (J/N)";
            std::cin.get(input, 3);
            std::cin.ignore(INT16_MAX, '\n');
            if (0==input[1] && ('J' == input[0] || 'j' == input[0])) {
                system("shutdown /r /t 0");
            }
            std::cout << std::endl << " Beim nächsten Start des Systems wird die Reparatur abgeschlossen.";
        }
#ifdef DEBUG
        std::cout << " Diese Ausführung dauerte " << clock() - start_time << " Ticks." << endl;
#endif // DEBUG
        std::cout << std::endl << std::endl << std::endl;
    }
    return 0;
}

void exec(string command) {
    string  line = command + " > process_" + to_string(counter) + ".txt 2>&1";
#ifdef DEBUG
    std::cout  << std::endl << "Command: " << line << endl;
#endif // DEBUG
    system(line.c_str());
    std::cout << "\r" << " Prozess " << counter++ << " von " << total << " abgeschlossen";
}

void printWarning(string warn) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    string header_centralline = "|     " + warn + "     |";
    string header_topline = "+" + string(header_centralline.size() - 2, '-') + "+";
    string header_padline = "|" + string(header_centralline.size() - 2, ' ') + "|";

    size_t startpoint = (columns - header_centralline.size()) / 2;

    cout << string(startpoint, ' ') << header_topline << endl;
    cout << string(startpoint, ' ') << header_padline << endl;
    cout << string(startpoint, ' ') << header_centralline << endl;
    cout << string(startpoint, ' ') << header_padline << endl;
    cout << string(startpoint, ' ') << header_topline << endl;
}