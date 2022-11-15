#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_resolver_mac_get_proxies_for_url(void *ctx, const char *url);
const char *proxy_resolver_mac_get_list(void *ctx);
bool proxy_resolver_mac_get_error(void *ctx, int32_t *error);
bool proxy_resolver_mac_wait(void *ctx, int32_t timeout_ms);
bool proxy_resolver_mac_cancel(void *ctx);

void *proxy_resolver_mac_create(void);
bool proxy_resolver_mac_delete(void **ctx);

bool proxy_resolver_mac_is_async(void);

bool proxy_resolver_mac_init(void);
bool proxy_resolver_mac_uninit(void);

proxy_resolver_i_s *proxy_resolver_mac_get_interface(void);

#ifdef __cplusplus
}
#endif
