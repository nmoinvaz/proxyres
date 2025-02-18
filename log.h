#include <stdarg.h>
#include <stdio.h>

void set_log_error(void (*func)(const char* fmt, ...));
void set_log_warn(void (*func)(const char* fmt, ...));
void set_log_info(void (*func)(const char* fmt, ...));
void set_log_debug(void (*func)(const char* fmt, ...));

void log_error(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_debug(const char *fmt, ...);

#define LOG_ERROR log_error
#define LOG_WARN log_warn
#define LOG_INFO log_info
#define LOG_DEBUG log_debug
