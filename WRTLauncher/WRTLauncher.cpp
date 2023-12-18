#include <iostream>
#include <Windows.h>
#include <winhttp.h>
#include "json.hpp"
#include <sstream> 
#include "../version.h"
#include <codecvt>
#include <fstream>
#include <errno.h>
#include <string.h>
#include <locale> 
#include <codecvt>

#pragma comment(lib, "winhttp.lib")

using namespace std;
using json = nlohmann::json;

/**
 * @brief Globale Varibale für Informationen über die aktuellste Version.
*/
json latest_verion_info;

/**
 * @brief Prüft ob auf Github eine neuere Version des WRT verfügbar ist. Speichert Infos in latest_version_info.
 * @return 1 wenn eine neue Version verfügbar ist, 0 wenn die Version aktuell ist, -1 bei Fehlern
*/
int check_for_updates() {
    HINTERNET session = 0, connection = 0, request = 0;

    DWORD size = 0, read = 0;
    session = WinHttpOpen(L"WRT 1.3.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, NULL); // winhttp session öffnen
    if (session) {
        connection = WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, NULL); //mit github api verbinden
    }
    else {
        cerr << " Konnte nicht auf Updates prüfen: Session nicht erstellt.\n";
        return -1;
    }
    if (connection) {
        // api request um das latest release zu bekommen
        request = WinHttpOpenRequest(connection, L"GET", L"repos/Layviz/WindowsReparaturTool/releases/latest", 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    }
    else {
        cerr << " Konnte nicht auf Updates prüfen: Connection nicht erstellt.\n";
        return -1;
    }
    // request schicken
    BOOL resp = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (resp) {
        resp = WinHttpReceiveResponse(request, NULL); // Antwort abwarten
    }
    else
    {
        cerr << " Konnte nicht auf Updates prüfen: Request nicht gesendet.\n";
        return -1;
    }
    if (resp) {
        // stringstream um die komplette Antwort darin abzuspeichern
        stringstream sstr = stringstream();

        // über Schleife den stream mit Daten aus der Antwort füllen.
        WinHttpQueryDataAvailable(request, &size);
        while (size > 0) {
            char* buffer = 0;
            buffer = new char[size + 1];
            memset(buffer, 0, size + 1);
            if (!buffer) {
                cerr << " Konnte nicht auf Updates prüfen: Kein Speicher!\n";
                return -1;
            }
            if (!WinHttpReadData(request, buffer, size, &read)) {
                cerr << " Konnte nicht auf Updates prüfen: Fehler beim Lesen\n";
                return -1;
            }
            sstr << buffer;

            delete[] buffer;
            WinHttpQueryDataAvailable(request, &size);
        }

        string resp_str = sstr.str();

        string version_str = "";
        try { // json parsen und Version auslesen
            latest_verion_info = json::parse(resp_str);
            if (latest_verion_info.contains("tag_name")) {
                version_str = latest_verion_info["tag_name"];
            }
        }
        catch (json::exception& e) {
            cerr << "Json Exception" << endl << "message: " << e.what() << endl << "id " << e.id << endl;
        }

        if (!version_str.empty()) {
            // Version aufspliten in Major, Minor und Patch Version
            if ('v' == version_str[0]) {
                version_str = version_str.substr(1);
            }
            size_t dot1 = version_str.find('.');
            size_t dot2 = version_str.find('.', dot1 + 1);
            string major_str = version_str.substr(0, dot1);
            string minor_str = version_str.substr(dot1 + 1, dot2 - (dot1 + 1));
            string patch_str = version_str.substr(dot2 + 1);

            int major = stoi(major_str);
            int minor = stoi(minor_str);
            int patch = stoi(patch_str);

            // Versionen vergleichen mit der über Makro eingestellten lokalen Version
            int cmp;
            if (major > VERSION_MAJOR) {
                cmp = 1;
            }
            else if (major < VERSION_MAJOR) {
                cmp = -1;
            }
            else { //major == VERSION_MAJOR
                if (minor > VERSION_MINOR) {
                    cmp = 1;
                }
                else if (minor < VERSION_MINOR) {
                    cmp = -1;
                }
                else {
                    if (patch > VERSION_PATCH) {
                        cmp = 1;
                    }
                    else if (patch < VERSION_PATCH) {
                        cmp = -1;
                    }
                    else {
                        cmp = 0;
                    }
                }
            }
            // entsprechende Antwort ausgeben
            if (1 == cmp) {
                std::cout << " Neue Version " << version_str << " gefunden." << endl << " Derzeitige Version ist " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << endl;
                return 1;
            }
            else if (0 == cmp) {
                //std::cout << " Derzeitige Version ist aktuell." << endl;
                return 0;
            }
            else {
                std::wcout << " Verion ist aktueller als Quelle." << endl;
                return 0;
            }
        }
        else {
            cerr << " Konnte nicht auf Updates prüfen. Aktuelle Version konnte nicht ermittelt werden.\n";
        }
    }
    return -1;
}

/**
 * @brief Lädt den Installer für die aktuellste Verion herunter und startet den Installer
 * @return -1 bei Fehlern, 0 wenn der Installer gestartet wurde.
*/
int download_installer(string *downloaded_file) {
    HINTERNET session = 0, connection = 0, request = 0;
    DWORD size = 0, read = 0, total = 0;

    session = WinHttpOpen(L"WRT 1.3.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, NULL);
    if (session) {
        connection = WinHttpConnect(session, L"github.com", INTERNET_DEFAULT_HTTPS_PORT, NULL);
    }
    else {
        cerr << " Konnte Update nicht herunterladen: Session nicht erstellt." << endl;
        return -1;
    }
    string request_path;
    string file_name;
    try
    {
        string download_url;
        auto asset = latest_verion_info["assets"][0];
        file_name = asset["name"];
        if (file_name.compare(file_name.length() - 4, 4, ".msi") == 0) {
            download_url = asset["browser_download_url"];
            size_t pos = 0;
            for (int i = 0; i < 3; i++) {
                pos = download_url.find('/', pos+1);
            }
            request_path = download_url.substr(pos);
        }
        else
        {
            cerr << " Konnte Update nicht herunterladen: Download URL nicht gefunden" << endl;
            return -1;
        }
        
    }
    catch (json::exception& e) {
        cerr << "Json Exception" << endl << "message: " << e.what() << endl << "id " << e.id << endl;
        return -1;
    }
    if (connection) {
        wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
        wstring temp = converter.from_bytes(request_path);
        //cout << "request " << request_path << endl;
        request = WinHttpOpenRequest(connection, L"GET", temp.c_str(), 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    }
    else {
        cerr << " Konnte Update nicht herunterladen: Connection nicht erstellt.\n";
        return -1;
    }
    BOOL resp = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (resp) {
        resp = WinHttpReceiveResponse(request, NULL);
    }
    else
    {
        cerr << " Konnte Update nicht herunterladen: Request nicht gesendet.\n";
        return -1;
    }
    if (resp) {
        
        ofstream file;
        file.open(file_name, ios::out | ios::binary | ios::trunc);

        WinHttpQueryDataAvailable(request, &size);
        while (size > 0) {
            char* buffer = 0;
            buffer = new char[size + 1];
            memset(buffer, 0, size + 1);
            if (!buffer) {
                cerr << " Konnte Update nicht herunterladen: Kein Speicher!" << endl;
                return -1;
            }
            if (!WinHttpReadData(request, buffer, size, &read)) {
                cerr << " Konnte Update nicht herunterladen: Fehler beim Lesen" << endl;
                return -1;
            }
            file.write(buffer, read);
            total += read;
            std::cout << " " << total << "bytes heruntergeladen               \r";
            delete[] buffer;
            WinHttpQueryDataAvailable(request, &size);
        }
        file.close();

        *downloaded_file = file_name;
        return 0;
    }
}

string get_exe_dir() {
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    wstring temp = std::wstring(buffer).substr(0, pos);

    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(temp);
}

int installation(string installer_file) {
    string install_dir = get_exe_dir();
    
    string cmd = "start \"WRT Update\" cmd /C \"C:\\Windows\\System32\\msiexec.exe /x ";
    cmd.append(PRODUCT_CODE);
    cmd.append(" /passive");

    cmd.append(" & ");

    cmd.append(installer_file);
    cmd.append(" TARGETDIR=\"");
    cmd.append(install_dir);
    cmd.append("\" /passive");
    cmd.append(" && start \"\" \"");
    cmd.append(install_dir);
    cmd.append("\\WRTLauncher.exe\"\"");
    std::cout << endl << " Deinstalliere alte Version und installiere neue Version " << endl;

    system(cmd.c_str());
    return 0;
}

int main(){
    SetConsoleOutputCP(65001);

    //Check for mutex
    HANDLE mutex = CreateMutex(NULL, false, L"Local\\WRT");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        cerr << "Das Programm wird bereits ausgeführt und kann nur einmal aufgerufen werden." << endl;
        cin.get();
        return -1;
    }

    int update_available = check_for_updates();
    // abfragen, ob man jetzt updaten, später oder beenden
    if (update_available > 0) {
        
        while (1) {
            std::cout << endl << " Update jetzt installieren?" << endl << endl;
            std::cout << " 1. Jetzt installieren." << endl;
            std::cout << " 2. Nächtes Mal erneut fragen." << endl;
            std::cout << " 3. Windows Reperatur Tool beenden." << endl << " ";

            int auswahl = 0;
            char input[3];
            std::cin.get(input, 3);
            if (input[1] == 0) {
                std::cin.ignore(INT16_MAX, '\n');
                auswahl = input[0] - 48;
            }
            if (1 == auswahl) {
                string* installer=new string;
                download_installer(installer);
                if (installer) {
                    installation(*installer);
                    return 0;
                }
                else {
                    std:cout << " Konnte Update nicht herunterladen." << endl;
                    char input[3];
                    std::cin.get(input, 3);
                    return -1;
                }
            }
            else if (2 == auswahl) {
                break;
            }
            else {
                return 0;
            }
        }
    }
    CloseHandle(mutex);
    int wrt = system("WRT.exe");
    if (wrt < 0) {
        char errorbuffer[94];
        strerror_s(errorbuffer, 0);
        cerr << " Fehler beim Starten von WRT: " << errorbuffer << endl;
        char dummy;
        cin.get(&dummy, 1);
    }
}