
#pragma once

#define MAX_PROXY_URL 256

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*proxy_resolver_resolved_cb)(void *ctx, void *user_data, int32_t error, const char *list);

bool proxy_resolver_get_proxies_for_url(void *ctx, const char *url);

bool proxy_resolver_get_list(void *ctx, char **list);
bool proxy_resolver_get_error(void *ctx, int32_t *error);
bool proxy_resolver_is_pending(void *ctx);
bool proxy_resolver_cancel(void *ctx);

bool proxy_resolver_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback);

bool proxy_resolver_create(void **ctx);
bool proxy_resolver_delete(void **ctx);

bool proxy_resolver_init(void);
bool proxy_resolver_uninit(void);

#ifdef __cplusplus
}
#endif
