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
    // Proxy list from system config
    char *list;
    // Next proxy pointer
    const char *listp;
} proxy_resolver_s;

static void proxy_resolver_get_proxies_for_url_threadpool(void *arg) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)arg;
    if (!proxy_resolver)
        return;
    g_proxy_resolver.proxy_resolver_i->discover_proxies_for_url(proxy_resolver->base, proxy_resolver->url);
}

static bool proxy_resolver_get_proxies_for_url_from_system_config(void *ctx, const char *url) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    char *auto_config_url = NULL;
    char *proxy = NULL;
    char *scheme = NULL;

    // Skip if auto-config url evaluation is required for proxy resolution
    auto_config_url = proxy_config_get_auto_config_url();
    if (auto_config_url)
        goto config_done;

    // Use scheme associated with the URL when determining proxy
    scheme = get_url_scheme(url, "http");
    if (!scheme) {
        LOG_ERROR("Unable to allocate memory for %s (%" PRId32 ")\n", "scheme");
        goto config_done;
    }

    // Check to see if manually configured proxy is specified in system config
    proxy = proxy_config_get_proxy(scheme);
    if (proxy) {
        // Check to see if we need to bypass the proxy for the url
        char *bypass_list = proxy_config_get_bypass_list();
        bool should_bypass = should_bypass_proxy(url, bypass_list);
        if (should_bypass) {
            // Bypass the proxy for the url
            LOG_INFO("Bypassing proxy for %s (%s)\n", url, bypass_list ? bypass_list : "null");
            proxy_resolver->list = strdup("direct://");
        } else {
            // Use proxy from settings
            proxy_resolver->list = get_url_from_host(url, proxy);
        }
        free(bypass_list);
    } else if (!proxy_config_get_auto_discover()) {
        // Use DIRECT connection since proxy auto-discovery is not necessary
        proxy_resolver->list = strdup("direct://");
    }

config_done:

    free(scheme);
    free(proxy);
    free(auto_config_url);

    return proxy_resolver->list != NULL;
}

bool proxy_resolver_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;

    proxy_resolver->listp = NULL;
    free(proxy_resolver->list);
    proxy_resolver->list = NULL;

    // Check if discover already takes into account system configuration
    if (!g_proxy_resolver.proxy_resolver_i->discover_uses_system_config) {
        // Check if auto-discovery is necessary
        if (proxy_resolver_get_proxies_for_url_from_system_config(ctx, url)) {
            // Use system proxy configuration if no auto-discovery mechanism is necessary
            return true;
        }
    }

    // Automatically discover proxy configuration asynchronously if supported, otherwise spool to thread pool
    if (g_proxy_resolver.proxy_resolver_i->discover_is_async)
        return g_proxy_resolver.proxy_resolver_i->discover_proxies_for_url(proxy_resolver->base, url);

    free(proxy_resolver->url);
    proxy_resolver->url = strdup(url);

    return threadpool_enqueue(g_proxy_resolver.threadpool, proxy_resolver,
                              proxy_resolver_get_proxies_for_url_threadpool);
}

const char *proxy_resolver_get_list(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    if (proxy_resolver->list)
        return proxy_resolver->list;
    return g_proxy_resolver.proxy_resolver_i->get_list(proxy_resolver->base);
}

char *proxy_resolver_get_next_proxy(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    if (!proxy_resolver->listp)
        return NULL;

    // Get the next proxy to connect through
    char *proxy = str_sep_dup(&proxy_resolver->listp, ",");
    if (!proxy) {
        if (proxy_resolver->list)
            proxy_resolver->listp = proxy_resolver->list;
        else
            proxy_resolver->listp = g_proxy_resolver.proxy_resolver_i->get_list(proxy_resolver->base);
    }
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
    if (proxy_resolver->list) {
        proxy_resolver->listp = proxy_resolver->list;
        return true;
    }
    if (g_proxy_resolver.proxy_resolver_i->wait(proxy_resolver->base, timeout_ms)) {
        proxy_resolver->listp = g_proxy_resolver.proxy_resolver_i->get_list(proxy_resolver->base);
        return true;
    }
    return false;
}

bool proxy_resolver_cancel(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    if (proxy_resolver->list)
        return true;
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
    free(proxy_resolver->url);
    free(proxy_resolver->list);
    g_proxy_resolver.proxy_resolver_i->delete (&proxy_resolver->base);
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
    if (g_proxy_resolver.proxy_resolver_i->is_discover_async) {
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
