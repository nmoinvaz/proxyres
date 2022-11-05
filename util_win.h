#pragma once

#ifdef __cplusplus
extern "C" {
#endif

wchar_t *utf8_dup_to_wchar(const char *src);
char *wchar_dup_to_utf8(const wchar_t *src);
char *get_proxy_by_protocol(const char *protocol, const char *proxy_list);
char *convert_proxy_list_to_uri_list(const char *proxy_list);

#ifdef __cplusplus
}
#endif
