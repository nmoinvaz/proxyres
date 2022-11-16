#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_execute_jsproxy_get_proxies_for_url(void *ctx, const char *script, const char *url);
const char *proxy_execute_jsproxy_get_list(void *ctx);
int32_t proxy_execute_jsproxy_get_error(void *ctx);

void *proxy_execute_jsproxy_create(void);
bool proxy_execute_jsproxy_delete(void **ctx);

bool proxy_execute_jsproxy_init(void);
bool proxy_execute_jsproxy_uninit(void);

proxy_execute_i_s *proxy_execute_jsproxy_get_interface(void);

#ifdef __cplusplus
}
#endif
