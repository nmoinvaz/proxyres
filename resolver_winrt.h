#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_resolver_winrt_get_proxies_for_url(void *ctx, const char *url);
const char *proxy_resolver_winrt_get_list(void *ctx);
int32_t proxy_resolver_winrt_get_error(void *ctx);
bool proxy_resolver_winrt_wait(void *ctx, int32_t timeout_ms);
bool proxy_resolver_winrt_cancel(void *ctx);

void *proxy_resolver_winrt_create(void);
bool proxy_resolver_winrt_delete(void **ctx);

bool proxy_resolver_winrt_is_async(void);

bool proxy_resolver_winrt_init(void);
bool proxy_resolver_winrt_uninit(void);

proxy_resolver_i_s *proxy_resolver_winrt_get_interface(void);

#ifdef __cplusplus
}
#endif