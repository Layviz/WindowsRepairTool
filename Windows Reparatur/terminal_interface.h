#pragma once

void setup_ui();
void show_logo();
void show_warning(wchar_t *warning);
bool query_repair_mode(wchar_t* query, wchar_t** modes, size_t num_modes, wchar_t* cancel_note, OPERATION* op);
bool read_user_intput(OPERATION* op);
void show_help(wchar_t *help_text);
int  query_pending_reboot(wchar_t* query, wchar_t *cancel_note, wchar_t** choices, size_t num_choices,wchar_t *help_text);
void show_error(const wchar_t* error, bool block);
void progress_note(wchar_t * note, wchar_t *additional_info, bool overwrite);
void add_progress_output(char* output,DWORD size);
void show_in_progress_note(wchar_t *note);
bool query_reboot_now(wchar_t *query, wchar_t *confirmations, wchar_t* reboot_planed);
void show_repair_finish_message(wchar_t* message);
void show_message(wchar_t* mesg);