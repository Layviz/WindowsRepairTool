#include <iostream>
#include <Windows.h>
#include <string>

using namespace std;

int total;
int counter;
void exec(string command);
void printWarning(string warn);

int main()
{
    SetConsoleOutputCP(65001);

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
        std::wcout << " System-Reparaturmodus wählen:" << std::endl << std::endl;
        std::cout << " 1. Einfache Reparatur    (Dauert wenige Minuten, kein Neustart erforderlich.)" << std::endl;
        std::cout << " 2. Standard Reparatur    (Kann mehrere Stunden dauern, Neustart erforderlich.)" << std::endl;
        std::cout << " 3. Erweiterte Reparatur  (Zusätzliche Reparaturen, falls die Standard Reparatur nicht ausreichend war, Neustart erforderlich.)" << std::endl;
        std::wcout << std::endl << " Beliebige Eingabe tätigen, um das Programm zu beenden." << std::endl << " ";

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