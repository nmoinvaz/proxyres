#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int32_t str_change_chr(char *str, char from, char to);
int32_t str_trim_end(char *str, char c);
char *str_find_first_char(char *str, char *chars);
char *str_find_char_safe(char *str, int32_t max_str, char c);

char *dns_resolve(const char *host, int32_t *error);
char *parse_url_host(const char *url);

#ifdef __cplusplus
}
#endif
