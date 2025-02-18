
// Define function pointers
#include "log.h"

static void (*log_error_func)(const char *fmt, ...);
static void (*log_warn_func)(const char *fmt, ...);
static void (*log_info_func)(const char *fmt, ...);
static void (*log_debug_func)(const char *fmt, ...);

void set_log_error(void (*func)(const char* fmt, ...)) {
    log_error_func = func;
}

void set_log_warn(void (*func)(const char* fmt, ...)) {
    log_warn_func = func;
}

void set_log_info(void (*func)(const char* fmt, ...)) {
    log_info_func = func;
}

void set_log_debug(void (*func)(const char* fmt, ...)) {
    log_debug_func = func;
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (log_error_func)
        log_error_func(fmt, args);
    else
        vprintf(fmt, args);
    va_end(args);
}
void log_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (log_warn_func)
        log_warn_func(fmt, args);
    else
        vprintf(fmt, args);
    va_end(args);
}
void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (log_info_func)
        log_info_func(fmt, args);
    else
        vprintf(fmt, args);
    va_end(args);
}
void log_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (log_debug_func)
        log_debug_func(fmt, args);
    else
        vprintf(fmt, args);
    va_end(args);
}
