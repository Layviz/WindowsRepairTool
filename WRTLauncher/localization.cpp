#include "../Shared/windows_support.h"
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
wchar_t error_receive[MAX_LOCALIZED_STRING_SIZE] = {};

void load_stings(HINSTANCE source) {
    wrt::load_string(source, MUTEX_WARN, mutex_warn, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_SESSION, error_seesion, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_CONNECTION, error_connection, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_REQUEST, error_request, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_MEMORY, error_memory, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_READ, error_read, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, VERSION_FOUND_FORMAT, version_found_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, VERSION_UP_TO_DATE, version_up_to_date, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, VERSION_AHEAD, version_ahead, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, VERSION_UNKNOWN, version_unknown, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_DOWNLOAD_URL, error_download_url, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, BYTES_READ, bytes_read, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, INSTALLER_START, installer_start, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, INSTALL_QUERY, install_query, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, INSTALL_MODE1, install_mode1, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, INSTALL_MODE2, install_mode2, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, INSTALL_MODE3, install_mode3, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_WRT, error_wrt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_UPDATE, error_update, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ERROR_RECEIVE, error_receive, MAX_LOCALIZED_STRING_SIZE);
}

int load_localized_strings() {
    return wrt::load_language(load_stings, CONSOLE_CODEPAGE);
}
