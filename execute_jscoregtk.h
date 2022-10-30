#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_execute_jscoregtk_get_proxies_for_url(void *ctx, const char *script, const char *url);
char *proxy_execute_jscoregtk_get_list(void *ctx);
bool proxy_execute_jscoregtk_get_error(void *ctx, int32_t *error);

void *proxy_execute_jscoregtk_create(void);
bool proxy_execute_jscoregtk_delete(void **ctx);

bool proxy_execute_jscoregtk_init(void);
bool proxy_execute_jscoregtk_uninit(void);

#ifdef __cplusplus
}
#endif
