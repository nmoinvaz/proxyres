#pragma once

#ifdef __cplusplus
extern "C" {
#endif

wchar_t *wstrdup(const char *src);
char *utf8strdup(const wchar_t *src);

#ifdef __cplusplus
}
#endif
