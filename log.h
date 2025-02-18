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

#define LOG_ERROR(fmt, ...) \
    log_error(fmt, ##__VA_ARGS__);
#define LOG_WARN(fmt, ...) \
    log_warn(fmt, ##__VA_ARGS__);
#define LOG_INFO(fmt, ...) \
    log_info(fmt, ##__VA_ARGS__);
#define LOG_DEBUG(fmt, ...) \
    log_debug(fmt, ##__VA_ARGS__);
