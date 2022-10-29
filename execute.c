#include <stdint.h>
#include <stdbool.h>

#if defined(__linux__)
#include "execute_jscoregtk.h"
#endif

bool proxy_execute_get_proxies_for_url(void *ctx, const char *script, const char *url) {
    return proxy_execute_jscoregtk_get_proxies_for_url(ctx, script, url);
}
char *proxy_execute_get_list(void *ctx) {
    return proxy_execute_jscoregtk_get_list(ctx);
}
bool proxy_execute_get_error(void *ctx, int32_t *error) {
    return proxy_execute_jscoregtk_get_error(ctx, error);
}

bool proxy_execute_create(void **ctx) {
    return proxy_execute_jscoregtk_create(ctx);
}
bool proxy_execute_delete(void *ctx) {
    return proxy_execute_jscoregtk_delete(ctx);
}

bool proxy_execute_init(void) {
    return proxy_execute_jscoregtk_init();
}

bool proxy_execute_uninit(void) {
    return proxy_execute_jscoregtk_uninit();
}
