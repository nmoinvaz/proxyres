
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


void default_log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("ERROR - ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void default_log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("WARNING - ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void default_log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("INFO - ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void default_log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("DEBUG - ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void log_error(const char *fmt, ...) {
    if (log_error_func)
        log_error(fmt);
    else
        default_log_error(fmt);
}
void log_warn(const char *fmt, ...)
{
    if (log_warn_func)
        log_warn(fmt);
    else
        default_log_warn(fmt);
}
void log_info(const char *fmt, ...)
{
    if (log_info_func)
        log_info(fmt);
    else
        default_log_info(fmt);
}
void log_debug(const char *fmt, ...)
{
    if (log_debug_func)
        log_debug(fmt);
    else
        default_log_debug(fmt);
}
