#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int32_t str_change_chr(char *str, char from, char to);
int32_t str_trim_end(char *str, char c);
const char *str_find_first_char(const char *str, const char *chars);
const char *str_find_len_first_char(const char *str, size_t str_len, const char *chars);
const char *str_find_len_char(const char *str, size_t str_len, char c);
const char *str_find_len_str(const char *str, size_t str_len, const char *find);
const char *str_find_len_case_str(const char *str, size_t str_len, const char *find);

char *dns_resolve(const char *host, int32_t *error);
char *get_url_host(const char *url);
const char *get_url_path(const char *url);
char *get_url_scheme(const char *url, const char *fallback);
char *convert_proxy_list_to_uri_list(const char *proxy_list);

#ifdef __cplusplus
}
#endif
