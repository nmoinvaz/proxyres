#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <errno.h>
#ifdef _WIN32
#  include <windows.h>
#endif

#include "config.h"
#include "log.h"
#include "resolver.h"
#include "resolver_i.h"
#include "resolver_posix.h"
#if defined(__APPLE__)
#  include "resolver_mac.h"
#elif defined(__linux__)
#  include "resolver_gnome3.h"
#elif defined(_WIN32)
#  if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
#    include "resolver_winxp.h"
#    include "resolver_win8.h"
#  elif WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
#    include "resolver_winrt.h"
#  endif
#endif
#include "threadpool.h"
#include "util.h"

typedef struct g_proxy_resolver_s {
    // Library reference count
    int32_t ref_count;
    // Proxy resolver interface
    proxy_resolver_i_s *proxy_resolver_i;
    // Thread pool
    void *threadpool;
} g_proxy_resolver_s;

g_proxy_resolver_s g_proxy_resolver;

typedef struct proxy_resolver_s {
    // Base proxy resolver instance
    void *base;
    // Async job
    char *url;
    // Next proxy pointer
    char *listp;
} proxy_resolver_s;

static void proxy_resolver_get_proxies_for_url_threadpool(void *arg) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)arg;
    if (!proxy_resolver)
        return;
    g_proxy_resolver.proxy_resolver_i->get_proxies_for_url(proxy_resolver->base, proxy_resolver->url);
}

bool proxy_resolver_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;

    proxy_resolver->listp = NULL;

    // Call get_proxies_for_url directly since the underlying interface is asynchronous
    if (g_proxy_resolver.proxy_resolver_i->is_async())
        return g_proxy_resolver.proxy_resolver_i->get_proxies_for_url(proxy_resolver->base, url);

    free(proxy_resolver->url);
    proxy_resolver->url = strdup(url);

    return threadpool_enqueue(g_proxy_resolver.threadpool, proxy_resolver,
                              proxy_resolver_get_proxies_for_url_threadpool);
}

const char *proxy_resolver_get_list(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    return (char *)g_proxy_resolver.proxy_resolver_i->get_list(proxy_resolver->base);
}

char *proxy_resolver_get_next_proxy(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    if (!proxy_resolver->listp)
        return NULL;

    // Get the next proxy to connect through
    char *proxy = str_sep_dup(&proxy_resolver->listp, ",");
    if (!proxy)
        proxy_resolver->listp = (char *)g_proxy_resolver.proxy_resolver_i->get_list(proxy_resolver->base);
    return proxy;
}

int32_t proxy_resolver_get_error(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return -1;
    return g_proxy_resolver.proxy_resolver_i->get_error(proxy_resolver->base);
}

bool proxy_resolver_wait(void *ctx, int32_t timeout_ms) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    if (g_proxy_resolver.proxy_resolver_i->wait(proxy_resolver->base, timeout_ms)) {
        proxy_resolver->listp = (char *)g_proxy_resolver.proxy_resolver_i->get_list(proxy_resolver->base);
        return true;
    }
    return false;
}

bool proxy_resolver_cancel(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    return g_proxy_resolver.proxy_resolver_i->cancel(proxy_resolver->base);
}

void *proxy_resolver_create(void) {
    if (!g_proxy_resolver.proxy_resolver_i)
        return NULL;
    proxy_resolver_s *proxy_resolver = calloc(1, sizeof(proxy_resolver_s));
    if (!proxy_resolver)
        return NULL;
    proxy_resolver->base = g_proxy_resolver.proxy_resolver_i->create();
    if (!proxy_resolver->base) {
        free(proxy_resolver);
        return NULL;
    }
    return proxy_resolver;
}

bool proxy_resolver_delete(void **ctx) {
    if (!g_proxy_resolver.proxy_resolver_i)
        return true;
    if (!ctx || !*ctx)
        return false;
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)*ctx;
    if (proxy_resolver->url)
        free(proxy_resolver->url);
    g_proxy_resolver.proxy_resolver_i->delete(&proxy_resolver->base);
    free(proxy_resolver);
    *ctx = NULL;
    return true;
}

bool proxy_resolver_global_init(void) {
    if (g_proxy_resolver.ref_count > 0) {
        g_proxy_resolver.ref_count++;
        return true;
    }
    memset(&g_proxy_resolver, 0, sizeof(g_proxy_resolver_s));
#if defined(_WIN32) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    WSADATA WsaData = {0};
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0) {
        LOG_ERROR("Failed to initialize winsock %d\n", WSAGetLastError());
        return false;
    }
#endif

    if (!proxy_config_global_init())
        return false;
#if defined(__APPLE__)
    if (proxy_resolver_mac_global_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_mac_get_interface();
#elif defined(__linux__)
        /* Does not work for manually specified proxy auto-config urls
        if (proxy_resolver_gnome3_global_init())
            g_proxy_resolver.proxy_resolver_i = proxy_resolver_gnome3_get_interface();*/
#elif defined(_WIN32)
#  if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
    if (proxy_resolver_win8_global_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_win8_get_interface();
    else if (proxy_resolver_winxp_global_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_winxp_get_interface();
#  elif WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
    if (proxy_resolver_winrt_global_init())
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_winrt_get_interface();
#  endif
#endif
#ifdef PROXYRES_EXECUTE
    if (!g_proxy_resolver.proxy_resolver_i)
        g_proxy_resolver.proxy_resolver_i = proxy_resolver_posix_get_interface();
#endif

    if (!g_proxy_resolver.proxy_resolver_i) {
        LOG_ERROR("No proxy resolver available\n");
        return false;
    }

    // No need to create thread pool since underlying implementation is already asynchronous
    if (g_proxy_resolver.proxy_resolver_i->is_async()) {
        g_proxy_resolver.ref_count++;
        return true;
    }

    // Create thread pool to handle proxy resolution requests asynchronously
    g_proxy_resolver.threadpool = threadpool_create(THREADPOOL_DEFAULT_MIN_THREADS, THREADPOOL_DEFAULT_MAX_THREADS);
    if (!g_proxy_resolver.threadpool) {
        LOG_ERROR("Failed to create thread pool\n");
        proxy_resolver_global_cleanup();
        return false;
    }

#ifdef PROXYRES_EXECUTE
    // Pass threadpool to posix resolver to immediately start wpad discovery
    if (g_proxy_resolver.proxy_resolver_i == proxy_resolver_posix_get_interface()) {
        if (!proxy_resolver_posix_init_ex(g_proxy_resolver.threadpool)) {
            LOG_ERROR("Failed to initialize posix proxy resolver\n");
            proxy_resolver_global_cleanup();
            return false;
        }
    }
#endif

    g_proxy_resolver.ref_count++;
    return true;
}

bool proxy_resolver_global_cleanup(void) {
    if (--g_proxy_resolver.ref_count > 0)
        return true;

    if (g_proxy_resolver.threadpool)
        threadpool_delete(&g_proxy_resolver.threadpool);

    if (g_proxy_resolver.proxy_resolver_i)
        g_proxy_resolver.proxy_resolver_i->global_cleanup();

    memset(&g_proxy_resolver, 0, sizeof(g_proxy_resolver));

    if (!proxy_config_global_cleanup())
        return false;

#if defined(_WIN32) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    WSACleanup();
#endif
    return true;
}
