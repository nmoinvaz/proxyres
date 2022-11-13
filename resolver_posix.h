#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_resolver_posix_get_proxies_for_url(void *ctx, const char *url);
const char *proxy_resolver_posix_get_list(void *ctx);
bool proxy_resolver_posix_get_error(void *ctx, int32_t *error);
bool proxy_resolver_posix_is_pending(void *ctx);
bool proxy_resolver_posix_cancel(void *ctx);

void *proxy_resolver_posix_create(void);
bool proxy_resolver_posix_delete(void **ctx);

bool proxy_resolver_posix_is_async(void);

bool proxy_resolver_posix_init(void);
bool proxy_resolver_posix_uninit(void);

proxy_resolver_i_s *proxy_resolver_posix_get_interface(void);

#ifdef __cplusplus
}
#endif
