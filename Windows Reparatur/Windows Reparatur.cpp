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
        "sfc /scannow",
        "Dism /Online /Cleanup-Image /ScanHealth",
        "sfc /scannow",
        "Dism /Online /Cleanup-Image /RestoreHealth",
        "sfc /scannow",
        "defrag C: /O /H"
    };
    string erweiterteReparatur[] = {
        "defrag C: /O /H",
        "sfc /scannow",
        "\"\"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe\" -Scan -ScanType 2\"",
        "sfc /scannow",
        "dism /online /cleanup-image /CheckHealth",
        "sfc /scannow",
        "Dism /Online /Cleanup-Image /ScanHealth",
        "sfc /scannow",
        "Dism /Online /Cleanup-Image /RestoreHealth",
        "sfc /scannow",
        "defrag C: /O /H",
        "chkdsk C: /f /r /x /b < bestaetigung.txt"
    };

    HANDLE mutex = CreateMutex(NULL, false, L"Local\\WRT");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        cerr << "Das Programm wird bereits ausgeführt und kann nur einmal aufgerufen werden." << endl;
        cin.get();
        return -1;
    }
    cout << endl;

    string header_warning = "Es wird empfohlen nebenbei keine aufwendigen Prozesse laufen zu lassen.";
    printWarning(header_warning);

    cout << endl << endl << endl;
    while (true) {
        int auswahl = 0;
        std::wcout << " System-Reparaturmodus wählen:" << std::endl << std::endl;
        std::cout << " 1. Standard Reparatur    (Dauert wenige Minuten, kein Neustart erforderlich.)" << std::endl;
        std::cout << " 2. Erweiterte Reparatur  (Kann mehrere Stunden dauern, Neustart erforderlich.)" << std::endl;
        std::wcout << std::endl << " Beliebige Eingabe tätigen, um das Programm zu beenden." << std::endl<<" ";
        
        char input[3];
        std::cin.get(input,3);
        if (input[1] == 0) {
            std::cin.ignore(INT16_MAX, '\n');
            auswahl = input[0] - 48;
        }

        if (auswahl != 1 && auswahl != 2) {
            break;
        }

        cout << endl << " Reparatur wird durchgeführt. Bitte nicht das Programm beenden oder den Computer ausschalten." << endl;

        if (auswahl == 1) {
            total = sizeof(standardReparatur) / sizeof(string);
        }
        else {
            total = sizeof(erweiterteReparatur) / sizeof(string);
        }
        counter = 1;
        
        std::cout << std::endl << " Prozess " << 1 << " von " << total << " gestartet";
        for (int i = 0; i < total; i++)
        {
            if (auswahl == 1)
            {
                exec(standardReparatur[i]);
            }
            else
            {
                exec(erweiterteReparatur[i]);
            }
        }
        if (2 == auswahl) {
            //system("chkdsk C: /f /r /x /b < bestaetigung.txt > nul 2>&1");
            std::cout << std::endl<<std::endl<< " Um die Reparatur abzuschließen ist ein Systemneustart erforderlich, Jetzt Neustarten? (J/N)";
            std::cin.get(input, 3);
            std::cin.ignore(INT16_MAX, '\n');
            if (0==input[1] && ('J' == input[0] || 'j' == input[0])) {
                system("shutdown /r /t 0");
            }
            std::cout << std::endl << " Beim nächsten Start des Systems wird die Reparatur abgeschlossen.";
        }
        std::cout << std::endl << std::endl << std::endl;
    }
    return 0;
}

void exec(string command) {
    string  line = command + " > nul 2>&1";
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