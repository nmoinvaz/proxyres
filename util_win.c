#include <stdint.h>
#include <wchar.h>

#include <windows.h>

// Create a wide char string from a UTF-8 string
wchar_t *utf8_dup_to_wchar(const char *src) {
    int32_t len = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    wchar_t *dup = (wchar_t *)calloc(len, sizeof(wchar_t));
    if (dup == NULL)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, src, -1, dup, len);
    return dup;
}

// Create a UTF-8 string from a wide char string
char *wchar_dup_to_utf8(const wchar_t *src) {
    int32_t len = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
    char *dup = (char *)calloc(len, sizeof(char));
    if (dup == NULL)
        return NULL;
    WideCharToMultiByte(CP_UTF8, 0, src, -1, dup, len, NULL, NULL);
    return dup;
}