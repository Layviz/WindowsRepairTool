#pragma once

// localized strings
#define MAX_LOCALIZED_STRING_SIZE 512
extern wchar_t error_seesion[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t error_connection[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t error_request[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t error_memory[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t error_read[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t version_found_fmt[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t version_up_to_date[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t version_ahead[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t version_unknown[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t error_download_url[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t bytes_read[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t installer_start[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t install_query[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t install_mode1[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t install_mode2[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t install_mode3[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t error_wrt[MAX_LOCALIZED_STRING_SIZE] ;

extern int load_localized_strings();