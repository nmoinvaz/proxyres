#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *dns_resolve(const char *host, int32_t *error);
char *parse_url_host(const char *url);

#ifdef __cplusplus
}
#endif
