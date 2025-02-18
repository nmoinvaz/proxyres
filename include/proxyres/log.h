#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Define function pointers
void set_log_error(void (*func)(const char* fmt, ...));

void set_log_warn(void (*func)(const char* fmt, ...));

void set_log_info(void (*func)(const char* fmt, ...));

void set_log_debug(void (*func)(const char* fmt, ...));

#ifdef __cplusplus
}
#endif
