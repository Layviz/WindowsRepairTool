#include <Windows.h>
#include <string>
#include "resource.h"

// localized strings
#define MAX_LOCALIZED_STRING_SIZE 2048
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
wchar_t help_text_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t pending_help_text[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t process_start_time_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t process_end_time_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t repair_time_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t seconds_str[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t minutes_str[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t hours_str[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t days_str[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t abort_msg[MAX_LOCALIZED_STRING_SIZE] = {};

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
    LoadString(source, HELP_TEXT_FMT, help_text_fmt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PENDING_HELP_TEXT, pending_help_text, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PROCESS_START_TIME_FMT, process_start_time_fmt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, PROCESS_END_TIME_FMT, process_end_time_fmt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, REPAIR_TIME_FMT, repair_time_fmt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, SECONDS_STR, seconds_str, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, MINUTES_STR, minutes_str, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, HOUR_STR, hours_str, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, DAYS_STR, days_str, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ABORT_MSG, abort_msg, MAX_LOCALIZED_STRING_SIZE);
}

int load_localized_strings() {
    wchar_t locale_name[3] = {};
    wchar_t dll_name[7] = {};
    wchar_t locale_dir_dll[15] = {};
    int ret;

    GetUserDefaultLocaleName(locale_name, 3);
    swprintf(dll_name, 7, L"%s.dll", locale_name);
    swprintf(locale_dir_dll, 15, L"locale\\%s.dll", locale_name);
    // try loading from locale dir
    HMODULE locale_source = LoadLibrary(locale_dir_dll);
    if (nullptr == locale_source) {
        // Try loading a lib with the local name
        locale_source = LoadLibrary(dll_name);
    }
    UINT codepage;
    if (nullptr == locale_source) {
        // if no locale lib exists use the default strings
        locale_source = GetModuleHandle(nullptr);
        codepage = 65001;
        ret = 1;
    }
    else
    {
        wchar_t codepage_str[16];
        LoadString(locale_source, CONSOLE_CODEPAGE, codepage_str, 16);
        codepage = wcstoul(codepage_str, NULL, 10);
        ret = 0;
    }

    SetConsoleOutputCP(codepage);
    load_stings(locale_source);
    return ret;
}