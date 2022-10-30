#include <stdint.h>
#include <stdbool.h>

#if defined(__linux__)
#include "execute_jscoregtk.h"
#elif defined(_WIN32)
#include "execute_jsproxy.h"
#endif

bool proxy_execute_get_proxies_for_url(void *ctx, const char *script, const char *url) {
#if defined(__linux__)
    return proxy_execute_jscoregtk_get_proxies_for_url(ctx, script, url);
#elif defined(_WIN32)
    return proxy_execute_jsproxy_get_proxies_for_url(ctx, script, url);
#endif
}

char *proxy_execute_get_list(void *ctx) {
#if defined(__linux__)
    return proxy_execute_jscoregtk_get_list(ctx);
#elif defined(_WIN32)
    return proxy_execute_jsproxy_get_list(ctx);
#endif
}
bool proxy_execute_get_error(void *ctx, int32_t *error) {
#if defined(__linux__)
    return proxy_execute_jscoregtk_get_error(ctx, error);
#elif defined(_WIN32)
    return proxy_execute_jsproxy_get_error(ctx, error);
#endif
}

void *proxy_execute_create(void) {
#if defined(__linux__)
    return proxy_execute_jscoregtk_create();
#elif defined(_WIN32)
    return proxy_execute_jsproxy_create();
#endif
}
bool proxy_execute_delete(void *ctx) {
#if defined(__linux__)
    return proxy_execute_jscoregtk_delete(ctx);
#elif defined(_WIN32)
    return proxy_execute_jsproxy_delete(ctx);
#endif
}

bool proxy_execute_init(void) {
#if defined(__linux__)
    return proxy_execute_jscoregtk_init();
#elif defined(_WIN32)
    return proxy_execute_jsproxy_init();
#endif
}

bool proxy_execute_uninit(void) {
#if defined(__linux__)
    return proxy_execute_jscoregtk_uninit();
#elif defined(_WIN32)
    return proxy_execute_jsproxy_uninit();
#endif
}
