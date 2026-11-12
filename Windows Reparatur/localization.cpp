#include "../Shared/windows_support.h"
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
wchar_t app_lock_error_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t pending_check_error_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t dism_skipped_note[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t invalid_selection[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t usage_text[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t step_status_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t step_progress_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t step_done_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t step_failed_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t step_skipped_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t summary_fmt[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t reboot_hint[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t confirm_hint[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t confirm_cleanup[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t confirm_surface_scan[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t confirm_offline_repair[MAX_LOCALIZED_STRING_SIZE] = {};
wchar_t help_batch_hint[MAX_LOCALIZED_STRING_SIZE] = {};

void load_stings(HINSTANCE source) {
    wrt::load_string(source, MUTEX_WARN, mutex_warn, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PENDING_RESTART_QUERY, pending_query, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PENDING_RESTART_OPTION1, pending_option1, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PENDING_RESTART_OPTION2, pending_option2, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PENDING_RESTART_OPTION3, pending_option3, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, STARTUP_WARNING, startup_warn, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, MODE_QUERY, mode_query, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, MODE_OPTION1, mode_option1, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, MODE_OPTION2, mode_option2, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, MODE_OPTION3, mode_option3, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, MODE_CANCEL_INFO, mode_cancel, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, IN_PROGRESS_NOTE, in_progress_note, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PROGRESS_STARTED_FORMAT_STRING, progress_started_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PROGESS_DONE_FORMAT_STRING, progress_done_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, REBOOT_QUERY, reboot_query, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, REBOOT_CONFIRMS, reboot_confirms, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, REBOOT_PLANNED, reboot_planned, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, EXECUTION_TIME_FORMAT_STRING, exec_time_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, HELP_TEXT_FMT, help_text_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PENDING_HELP_TEXT, pending_help_text, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PROCESS_START_TIME_FMT, process_start_time_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PROCESS_END_TIME_FMT, process_end_time_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, REPAIR_TIME_FMT, repair_time_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, SECONDS_STR, seconds_str, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, MINUTES_STR, minutes_str, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, HOUR_STR, hours_str, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, DAYS_STR, days_str, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, ABORT_MSG, abort_msg, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, APP_LOCK_ERROR_FMT, app_lock_error_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, PENDING_CHECK_ERROR_FMT, pending_check_error_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, DISM_SKIPPED_NOTE, dism_skipped_note, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, INVALID_SELECTION, invalid_selection, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, USAGE_TEXT, usage_text, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, STEP_STATUS_FMT, step_status_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, STEP_PROGRESS_FMT, step_progress_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, STEP_DONE_FMT, step_done_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, STEP_FAILED_FMT, step_failed_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, STEP_SKIPPED_FMT, step_skipped_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, SUMMARY_FMT, summary_fmt, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, REBOOT_HINT, reboot_hint, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, CONFIRM_HINT, confirm_hint, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, CONFIRM_CLEANUP, confirm_cleanup, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, CONFIRM_SURFACE_SCAN, confirm_surface_scan, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, CONFIRM_OFFLINE_REPAIR, confirm_offline_repair, MAX_LOCALIZED_STRING_SIZE);
    wrt::load_string(source, HELP_BATCH_HINT, help_batch_hint, MAX_LOCALIZED_STRING_SIZE);
}

int load_localized_strings() {
    return wrt::load_language(load_stings, CONSOLE_CODEPAGE);
}
