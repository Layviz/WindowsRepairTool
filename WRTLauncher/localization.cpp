#include <Windows.h>
#include <string>
#include "resource.h"

// localized strings
#define MAX_LOCALIZED_STRING_SIZE 512
wchar_t mutex_warn[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_seesion[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_connection[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_request[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_memory[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_read[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t version_found_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t version_up_to_date[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t version_ahead[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t version_unknown[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_download_url[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t bytes_read[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t installer_start[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t install_query[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t install_mode1[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t install_mode2[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t install_mode3[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_wrt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t error_update[MAX_LOCALIZED_STRING_SIZE] = {};

void load_stings(HINSTANCE source) {
    LoadString(source, MUTEX_WARN, mutex_warn, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_SESSION, error_seesion, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_CONNECTION, error_connection, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_REQUEST, error_request, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_MEMORY, error_memory, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_READ, error_read, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, VERSION_FOUND_FORMAT, version_found_fmt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, VERSION_UP_TO_DATE, version_up_to_date, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, VERSION_AHEAD, version_ahead, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, VERSION_UNKNOWN, version_unknown, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_DOWNLOAD_URL, error_download_url, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, BYTES_READ, bytes_read, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, INSTALLER_START, installer_start, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, INSTALL_QUERY, install_query, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, INSTALL_MODE1, install_mode1, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, INSTALL_MODE2, install_mode2, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, INSTALL_MODE3, install_mode3, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_WRT, error_wrt, MAX_LOCALIZED_STRING_SIZE);
    LoadString(source, ERROR_UPDATE, error_update, MAX_LOCALIZED_STRING_SIZE);
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
        HMODULE locale_source = LoadLibrary(dll_name);
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
        wchar_t codepage_str[16] = {};
        LoadString(locale_source, CONSOLE_CODEPAGE, codepage_str, 16);
        codepage = wcstoul(codepage_str, NULL, 10);
        ret = 0;
    }

    SetConsoleOutputCP(codepage);
    load_stings(locale_source);
    return ret;
}