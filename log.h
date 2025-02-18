#pragma once

#include <cstdio>
#include <cstdarg>

// Define function pointers
void (*log_error)(const char *fmt, ...);
void (*log_warn)(const char *fmt, ...);
void (*log_info)(const char *fmt, ...);
void (*log_debug)(const char *fmt, ...);

void set_log_error(void (*func)(const char* fmt, ...)) {
    log_error = func;
}

void set_log_warn(void (*func)(const char* fmt, ...)) {
    log_warn = func;
}

void set_log_info(void (*func)(const char* fmt, ...)) {
    log_info = func;
}

void set_log_debug(void (*func)(const char* fmt, ...)) {
    log_debug = func;
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

#define LOG_ERROR(fmt, ...) \
  if (log_error) log_error(fmt, ##__VA_ARGS__);
#define LOG_WARN(fmt, ...) \
  if (log_warn) log_warn(fmt, ##__VA_ARGS__);
#ifdef _DEBUG
#  define LOG_INFO(fmt, ...) \
    if (log_info) log_info(fmt, ##__VA_ARGS__);
#  define LOG_DEBUG(fmt, ...) \
    if (log_debug) log_debug(fmt, ##__VA_ARGS__);
#else
#  define LOG_INFO(fmt, ...)
#  define LOG_DEBUG(fmt, ...)
#endif
