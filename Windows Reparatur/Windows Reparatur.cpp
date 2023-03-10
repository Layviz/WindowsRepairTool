#include <iostream>
#include <Windows.h>

int main()
{
    SetConsoleOutputCP(65001);
    while (true) {
        int auswahl = 0;
        std::wcout << " System-Reparaturmodus wählen:" << std::endl << std::endl;
        std::cout << " 1. Standard Reparatur" << std::endl;
        std::cout << " 2. Erweiterte Reparatur (Neustart erforderlich)" << std::endl;
        std::wcout << std::endl << " Beliebige Eingabe tätigen, um das Programm zu beenden." << std::endl<<" ";
        char input[3];
        std::cin.get(input,3);
        if (input[1] == 0) {
            std::cin.ignore(INT16_MAX, '\n');
            auswahl = input[0] - 48;
        }

        if (auswahl != 1 && auswahl != 2) {
            return 1;
        }
        int total = 4 + auswahl;

        std::cout << std::endl << " Prozess 1 von " << total << " gestartet";
        system("sfc /scannow > nul 2>&1");
        std::cout << "\r" << " Prozess 1 von " << total << " abgeschlossen";
        system("Dism /Online /Cleanup-Image /ScanHealth > nul 2>&1");
        std::cout << "\r" << " Prozess 2 von " << total << " abgeschlossen";
        system("sfc /scannow > nul 2>&1");
        std::cout << "\r" << " Prozess 3 von " << total << " abgeschlossen";
        system("Dism /Online /Cleanup-Image /RestoreHealth > nul 2>&1");
        std::cout << "\r" << " Prozess 4 von " << total << " abgeschlossen";
        system("sfc /scannow > nul 2>&1");
        std::cout << "\r" << " Prozess 5 von " << total << " abgeschlossen";
        if (2 == auswahl) {
            system("chkdsk C: /f /r /x /b < bestaetigung.txt > nul 2>&1");
            std::cout << "\r" << " Prozess 6 von " << total << " abgeschlossen" << std::endl<<std::endl;
            std::cout << " Um die Reparatur abzuschließen ist ein Systemneustart erforderlich, Jetzt Neustarten? (J/N)";
            std::cin.get(input, 3);
            std::cin.ignore(INT16_MAX, '\n');
            if (0==input[1] && ('J' == input[0] || 'j' == input[0])) {
                system("shutdown /r /t 0");
            }
            std::cout << std::endl << " Beim nächsten Start des Systems wird die Reparatur abgeschlossen.";
        }
        std::cout << std::endl << std::endl << std::endl;
    }
}