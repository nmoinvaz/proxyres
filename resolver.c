#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>

#include "resolver.h"
#include "resolver_i.h"

#if defined(__APPLE__)
#  include "resolver_mac.h"
#elif defined(__linux__)
#  include "resolver_gnome3.h"
#elif defined(_WIN32)
#  include "resolver_winxp.h"
#  include "resolver_win8.h"
#endif

typedef struct g_proxy_resolver_s {
    // Proxy resolver interface
    proxy_resolver_i_s *proxy_resolver_i;
    // Resolve callback
    void *user_data;
    proxy_resolver_resolved_cb callback;
} g_proxy_resolver_s;

g_proxy_resolver_s g_proxy_resolver;

bool proxy_resolver_get_proxies_for_url(void *ctx, const char *url) {
    if (g_proxy_resolver.proxy_resolver_i)
        return g_proxy_resolver.proxy_resolver_i->get_proxies_for_url(ctx, url);

    // Trigger resolve callback
    if (g_proxy_resolver.callback)
        g_proxy_resolver.callback(ctx, g_proxy_resolver.user_data, ENOSYS, NULL);

    return false;
}

bool proxy_resolver_get_list(void *ctx, char **list) {
    if (g_proxy_resolver.proxy_resolver_i)
        return g_proxy_resolver.proxy_resolver_i->get_list(ctx, list);
    if (!list)
        return false;
    *list = NULL;
    return (*list == NULL);
}

bool proxy_resolver_get_error(void *ctx, int32_t *error) {
    if (g_proxy_resolver.proxy_resolver_i)
        return g_proxy_resolver.proxy_resolver_i->get_error(ctx, error);
    if (!error)
        return false;
    *error = ENOSYS;
    return true;
}

bool proxy_resolver_is_pending(void *ctx) {
    if (!g_proxy_resolver.proxy_resolver_i)
        return false;
    return g_proxy_resolver.proxy_resolver_i->is_pending(ctx);
}

bool proxy_resolver_cancel(void *ctx) {
    if (!g_proxy_resolver.proxy_resolver_i)
        return false;
    return g_proxy_resolver.proxy_resolver_i->cancel(ctx);
}

bool proxy_resolver_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    if (g_proxy_resolver.proxy_resolver_i)
        return g_proxy_resolver.proxy_resolver_i->set_resolved_callback(ctx, user_data, callback);
    g_proxy_resolver.user_data = user_data;
    g_proxy_resolver.callback = callback;
    return true;
}

bool proxy_resolver_create(void **ctx) {
    if (!g_proxy_resolver.proxy_resolver_i) {
        if (*ctx != NULL)
            *ctx = &g_proxy_resolver;
        return true;
    }
    return g_proxy_resolver.proxy_resolver_i->create(ctx);
}

bool proxy_resolver_delete(void **ctx) {
    if (!g_proxy_resolver.proxy_resolver_i)
        return true;
    return g_proxy_resolver.proxy_resolver_i->delete(ctx);
}

bool proxy_resolver_init(void) {
#if defined(__APPLE__)
    if (proxy_resolver_mac_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_mac_get_interface();
#elif defined(__linux__)
    if (proxy_resolver_gnome3_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_gnome3_get_interface();
#elif defined(_WIN32)
    if (proxy_resolver_win8_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_win8_get_interface();
    else if (proxy_resolver_winxp_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_winxp_get_interface();
#endif
    return false;
}

bool proxy_resolver_uninit(void) {
    if (g_proxy_resolver.proxy_resolver_i)
        return g_proxy_resolver.proxy_resolver_i->uninit();

    memset(&g_proxy_resolver, 0, sizeof(g_proxy_resolver));
    return false;
}
