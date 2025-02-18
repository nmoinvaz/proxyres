#pragma once

// Define function pointers
void set_log_error(void (*func)(const char* fmt, ...));

void set_log_warn(void (*func)(const char* fmt, ...));

void set_log_info(void (*func)(const char* fmt, ...));

void set_log_debug(void (*func)(const char* fmt, ...));