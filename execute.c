#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "execute.h"
#include "execute_i.h"
#ifdef _WIN32
#ifdef HAVE_JSPROXY
#include "execute_jsproxy.h"
#endif
#if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
#include "execute_wsh.h"
#endif
#else
#include "execute_jscore.h"
#endif

typedef struct g_proxy_execute_s {
    // Library reference count
    int32_t ref_count;
    // Proxy execute interface
    proxy_execute_i_s *proxy_execute_i;
} g_proxy_execute_s;

g_proxy_execute_s g_proxy_execute;

bool proxy_execute_get_proxies_for_url(void *ctx, const char *script, const char *url) {
    if (!g_proxy_execute.proxy_execute_i)
        return false;
    return g_proxy_execute.proxy_execute_i->get_proxies_for_url(ctx, script, url);
}

const char *proxy_execute_get_list(void *ctx) {
    if (!g_proxy_execute.proxy_execute_i)
        return NULL;
    return g_proxy_execute.proxy_execute_i->get_list(ctx);
}

int32_t proxy_execute_get_error(void *ctx) {
    if (!g_proxy_execute.proxy_execute_i)
        return -1;
    return g_proxy_execute.proxy_execute_i->get_error(ctx);
}

void *proxy_execute_create(void) {
    if (!g_proxy_execute.proxy_execute_i)
        return NULL;
    return g_proxy_execute.proxy_execute_i->create();
}

bool proxy_execute_delete(void **ctx) {
    if (!g_proxy_execute.proxy_execute_i)
        return false;
    return g_proxy_execute.proxy_execute_i->delete(ctx);
}

bool proxy_execute_init(void) {
    if (g_proxy_execute.ref_count > 0) {
        g_proxy_execute.ref_count++;
        return true;
    }
    memset(&g_proxy_execute, 0, sizeof(g_proxy_execute));
#ifdef _WIN32
#if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
    if (proxy_execute_wsh_init())
        g_proxy_execute.proxy_execute_i = proxy_execute_wsh_get_interface();
#ifdef HAVE_JSPROXY
    else if (proxy_execute_jsproxy_init())
        g_proxy_execute.proxy_execute_i = proxy_execute_jsproxy_get_interface();
#endif
#endif
#else
    if (proxy_execute_jscore_init())
        g_proxy_execute.proxy_execute_i = proxy_execute_jscore_get_interface();
#endif
    if (!g_proxy_execute.proxy_execute_i)
        return false;
    g_proxy_execute.ref_count++;
    return true;
}

bool proxy_execute_uninit(void) {
    if (--g_proxy_execute.ref_count > 0)
        return true;
    if (g_proxy_execute.proxy_execute_i)
        g_proxy_execute.proxy_execute_i->uninit();

    memset(&g_proxy_execute, 0, sizeof(g_proxy_execute));
    return true;
}
