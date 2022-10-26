#pragma once

#ifdef __cplusplus
extern "C" {
#endif

const wchar_t *wstrdup(const char *src);
const char *utf8strdup(const wchar_t *src);

#ifdef __cplusplus
}
#endif
