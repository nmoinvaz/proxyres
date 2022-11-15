
#pragma once

#define MAX_PROXY_URL 256

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_resolver_get_proxies_for_url(void *ctx, const char *url);
const char *proxy_resolver_get_list(void *ctx);
bool proxy_resolver_get_error(void *ctx, int32_t *error);
bool proxy_resolver_wait(void *ctx, int32_t timeout_ms);
bool proxy_resolver_cancel(void *ctx);

void *proxy_resolver_create(void);
bool proxy_resolver_delete(void **ctx);

bool proxy_resolver_init(void);
bool proxy_resolver_uninit(void);

#ifdef __cplusplus
}
#endif
