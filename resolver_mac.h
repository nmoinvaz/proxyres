#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_resolver_mac_get_proxies_for_url(void *ctx, const char *url);

bool proxy_resolver_mac_get_list(void *ctx, char **list);
bool proxy_resolver_mac_get_error(void *ctx, int32_t *error);
bool proxy_resolver_mac_is_pending(void *ctx);
bool proxy_resolver_mac_cancel(void *ctx);

bool proxy_resolver_mac_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback);

bool proxy_resolver_mac_create(void **ctx);
bool proxy_resolver_mac_delete(void **ctx);

bool proxy_resolver_mac_init(void);
bool proxy_resolver_mac_uninit(void);

proxy_resolver_i_s *proxy_resolver_mac_get_interface(void);

#ifdef __cplusplus
}
#endif
