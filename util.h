#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *dns_resolve(const char *host, int32_t *error);
char *parse_url_host(const char *url);
char *get_config_value(const char *config, const char *section, const char *key);

#ifdef __cplusplus
}
#endif
