#pragma once

#ifdef __cplusplus
extern "C" {
#endif

wchar_t *utf8_dup_to_wchar(const char *src);
char *wchar_dup_to_utf8(const wchar_t *src);

#ifdef __cplusplus
}
#endif
