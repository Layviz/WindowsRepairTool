#include <iostream>
#include <Windows.h>
#include <string>
#include <time.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include "wrt_types.h"

using namespace std;

void set_console_font() {
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 18;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Cascadia Code");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
}

void setup_ui() {
    set_console_font();
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
    int columns, rows;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    wstring space = wstring(columns / 2 - 28, ' ');

    _setmode(_fileno(stdout), _O_U16TEXT);
    wprintf(space.c_str());
    wprintf(L"                                               __   __\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L"__                __  ______    ___________   ///   \\\\\\\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L"\\\\\\      __      /// ||||||\\\\  |\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263\u2263|  \\\\\\___///\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L" \\\\\\    //\\\\    ///  |||__)||)      |||        \u25e5\u25a9\u2263\u2263\u2263\u25a9\u25e4\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L"  \\\\\\  ///\\\\\\  ///   ||||||//       |||        ////\u203e\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L"   \\\\\\////\\\\\\\\///    |||  \\\\\\       |||       ////\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L"    \\\\\\//  \\\\///     |||   \\\\\\      |||      ////\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L"     \\\\/    \\//      |||    \\\\\\     |||     ////\n");
    fflush(stdout);
    Sleep(wait);
    wprintf(space.c_str());
    wprintf(L"      \u203e      \u203e        \u203e      \u203e\u203e      \u203e      \\\u2263/\n");
    fflush(stdout);
    Sleep(final_wait);
    space = wstring(rows - 18, '\n');
    wprintf(space.c_str());
    for (int i = 0; i < 9; i++)
    {
        wprintf(L"\n");
        Sleep(wait);
    }
}



void show_warning(wchar_t* warning) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns;
    wstring warn = warning;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    wstring header_centralline = L"|     " + warn + L"     |";
    wstring header_topline = L"+" + wstring(header_centralline.size() - 2, '-') + L"+";
    wstring header_padline = L"|" + wstring(header_centralline.size() - 2, ' ') + L"|";

    size_t startpoint = (columns - header_centralline.size()) / 2;

    std::wcout << wstring(startpoint, ' ') << header_topline << endl;
    std::wcout << wstring(startpoint, ' ') << header_padline << endl;
    std::wcout << wstring(startpoint, ' ') << header_centralline << endl;
    std::wcout << wstring(startpoint, ' ') << header_padline << endl;
    std::wcout << wstring(startpoint, ' ') << header_topline << endl;

    std::wcout << endl << endl << endl;
}

bool read_user_intput(OPERATION * op){
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

bool query_repair_mode(wchar_t* query, wchar_t** modes, size_t num_modes, wchar_t* cancel_note, OPERATION* op) {

    std::wcout << L" " << query << std::endl << std::endl;
    for (int i = 0; i < num_modes; i++) {
        std::wcout << L" " << modes[i] << endl;
    }
    std::wcout << std::endl << L" " << cancel_note << std::endl << L" ";
    return read_user_intput(op);
}

int query_pending_reboot(wchar_t* query, wchar_t *cancel_note, wchar_t** choices, size_t num_choices, wchar_t *help_text) {
    std::wcout << L" " << query << endl << endl;
    for (int i = 0; i < num_choices; i++) {
        std::wcout << L" " << choices[i] << endl;
    }
    char input[4] = {};
    int auswahl = 0;

    do {
        std::wcout << std::endl << L" " << cancel_note << std::endl << L" ";
        std::cin.get(input, 3);
        if (input[1] == 0) {
            std::cin.ignore(INT16_MAX, '\n');
            auswahl = input[0] - 48;
        }
        if ('h' == input[0]) {
            std::wcout << endl << help_text;

        }

    } while ('h' == input[0]);
    return auswahl;
}

void show_help(wchar_t* help_text) {
    std::wcout << endl << endl << help_text << L" ";
}

void show_error(const wchar_t* error, bool block) {
    std::wcerr << L" " << error << endl;
    if (block)
        cin.get();
}

void progress_note(wchar_t* note, wchar_t* additional_info, bool overwrite) {
    wstring blank_line(80, L' '); //80 spaces should be enough to overwrite everthing
    if (overwrite)
        wcout << L"\r" << blank_line << L"\r";
    wcout << note;
    if (additional_info)
        wcout << additional_info;
    if (!overwrite)
        wcout << L"\n";
}

void add_progress_output(char* output, DWORD size){
    cout << output;
}

void show_in_progress_note(wchar_t *note) {
    std::wcout << endl << L" " << note << endl;
}

bool query_reboot_now(wchar_t *query, wchar_t *confirmations, wchar_t *reboot_planed) {
    char input[4] = {};
    std::wcout << std::endl << std::endl << L" " << query;
    std::cin.get(input, 3);
    std::cin.ignore(INT16_MAX, '\n');
    if (0 == input[1]) {
        size_t confirm_len = wcslen(confirmations);
        for (int i = 0; i < confirm_len; i++) {
            if (input[0] == confirmations[i]) {
                return true;
            }
        }
    }
    std::wcout << std::endl << L" " << reboot_planed << endl;
    return false;
}

void show_repair_finish_message(wchar_t* message) {
    if(message)
        std::wcout << L" " << message << endl;
    std::wcout << std::endl << std::endl << std::endl;
}

void show_message(wchar_t* mesg) {
    std::wcout << endl << L" " << mesg << endl;
}