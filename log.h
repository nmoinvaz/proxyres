#pragma once

#define LOG_ERROR(fmt, ...) \
    { printf("ERROR - "), printf(fmt, ##__VA_ARGS__); }
#define LOG_WARN(fmt, ...) \
    { printf("WARNING - "), printf(fmt, ##__VA_ARGS__); }
#ifdef _DEBUG
#define LOG_INFO(fmt, ...) \
    { printf("INFO - "), printf(fmt, ##__VA_ARGS__); }
#define LOG_DEBUG(fmt, ...) \
    { printf("DEBUG - "), printf(fmt, ##__VA_ARGS__); }
#else
#define LOG_INFO(fmt, ...)
#define LOG_DEBUG(fmt, ...)
#endif
