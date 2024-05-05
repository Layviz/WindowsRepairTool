#pragma once

// localized strings
#define MAX_LOCALIZED_STRING_SIZE 2048
extern wchar_t mutex_warn[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t pending_query[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t pending_option1[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t pending_option2[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t pending_option3[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t startup_warn[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t mode_query[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t mode_option1[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t mode_option2[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t mode_option3[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t mode_cancel[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t in_progress_note[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t progress_started_fmt[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t progress_done_fmt[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t reboot_query[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t reboot_confirms[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t reboot_planned[MAX_LOCALIZED_STRING_SIZE] ;
extern wchar_t exec_time_fmt[MAX_LOCALIZED_STRING_SIZE];
extern wchar_t help_text_fmt[MAX_LOCALIZED_STRING_SIZE];
extern wchar_t pending_help_text[MAX_LOCALIZED_STRING_SIZE];
extern wchar_t process_start_time_fmt[MAX_LOCALIZED_STRING_SIZE];
extern wchar_t process_end_time_fmt[MAX_LOCALIZED_STRING_SIZE];
extern wchar_t repair_time_fmt[MAX_LOCALIZED_STRING_SIZE];


extern int load_localized_strings();