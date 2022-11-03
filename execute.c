#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include "execute_jsproxy.h"
#else
#include "execute_jscore.h"
#endif

bool proxy_execute_get_proxies_for_url(void *ctx, const char *script, const char *url) {
#ifdef _WIN32
    return proxy_execute_jsproxy_get_proxies_for_url(ctx, script, url);
#else
    return proxy_execute_jscore_get_proxies_for_url(ctx, script, url);
#endif
}

const char *proxy_execute_get_list(void *ctx) {
#ifdef _WIN32
    return proxy_execute_jsproxy_get_list(ctx);
#else
    return proxy_execute_jscore_get_list(ctx);
#endif
}

bool proxy_execute_get_error(void *ctx, int32_t *error) {
#ifdef _WIN32
    return proxy_execute_jsproxy_get_error(ctx, error);
#else
    return proxy_execute_jscore_get_error(ctx, error);
#endif
}

void *proxy_execute_create(void) {
#ifdef _WIN32
    return proxy_execute_jsproxy_create();
#else
    return proxy_execute_jscore_create();
#endif
}
bool proxy_execute_delete(void *ctx) {
#ifdef _WIN32
    return proxy_execute_jsproxy_delete(ctx);
#else
    return proxy_execute_jscore_delete(ctx);
#endif
}

bool proxy_execute_init(void) {
#ifdef _WIN32
    return proxy_execute_jsproxy_init();
#else
    return proxy_execute_jscore_init();
#endif
}

bool proxy_execute_uninit(void) {
#ifdef _WIN32
    return proxy_execute_jsproxy_uninit();
#else
    return proxy_execute_jscore_uninit();
#endif
}
