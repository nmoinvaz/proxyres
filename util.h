#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int32_t str_change_chr(char *str, char from, char to);
int32_t str_trim_end(char *str, char c);

char *dns_resolve(const char *host, int32_t *error);
char *parse_url_host(const char *url);

#ifdef __cplusplus
}
#endif
