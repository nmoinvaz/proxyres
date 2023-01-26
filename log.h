#pragma once

#define UNUSED1(p)                                       (void)(p)
#define UNUSED2(p1, p2)                                  UNUSED1(p1), UNUSED1(p2)
#define UNUSED3(p1, p2, p3)                              UNUSED1(p1), UNUSED2(p2, p3)
#define UNUSED4(p1, p2, p3, p4)                          UNUSED2(p1, p2), UNUSED2(p3, p4)
#define UNUSED5(p1, p2, p3, p4, p5)                      UNUSED2(p1, p2), UNUSED3(p3, p4, p5)
#define UNUSED6(p1, p2, p3, p4, p5, p6)                  UNUSED3(p1, p2, p3), UNUSED3(p4, p5, p6)
#define UNUSEDN(n)                                       UNUSED##n

#define UNUSED_ARGC_IMPL(p1, p2, p3, p4, p5, p6, n, ...) n
#define UNUSED_ARGC(...)                                 UNUSED_ARGC_IMPL(__VA_ARGS__, 6, 5, 4, 3, 2, 1)

#define UNUSED_IMPL(n)                                   UNUSEDN(n)
#define UNUSED(...)                                      UNUSED_IMPL(UNUSED_ARGC(__VA_ARGS__))(__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
  { printf("ERROR - "), printf(fmt, ##__VA_ARGS__); }
#define LOG_WARN(fmt, ...) \
  { printf("WARNING - "), printf(fmt, ##__VA_ARGS__); }
#ifdef _DEBUG
#  define LOG_INFO(fmt, ...) \
    { printf("INFO - "), printf(fmt, ##__VA_ARGS__); }
#  define LOG_DEBUG(fmt, ...) \
    { printf("DEBUG - "), printf(fmt, ##__VA_ARGS__); }
#else
#  define LOG_INFO(fmt, ...)  UNUSED(fmt, ##__VA_ARGS__)
#  define LOG_DEBUG(fmt, ...) UNUSED(fmt, ##__VA_ARGS__)
#endif
