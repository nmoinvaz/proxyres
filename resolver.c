#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include "log.h"
#include "resolver.h"
#include "resolver_i.h"
#if defined(__APPLE__)
#include "resolver_mac.h"
#elif defined(__linux__)
#include "resolver_gnome3.h"
#elif defined(_WIN32)
#include "resolver_winxp.h"
#include "resolver_win8.h"
#endif
#include "threadpool_pthread.h"

typedef struct g_proxy_resolver_s {
    // Proxy resolver interface
    proxy_resolver_i_s *proxy_resolver_i;
    // Thread pool
    void *thread_pool;
} g_proxy_resolver_s;

g_proxy_resolver_s g_proxy_resolver;

typedef struct proxy_resolver_s {
    // Base proxy resolver instance
    void *base;
    // Async job
    char *url;
    bool pending;
} proxy_resolver_s;

static void proxy_resolver_get_proxies_for_url_threadpool(void *arg) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)arg;
    if (!proxy_resolver)
        return;

    g_proxy_resolver.proxy_resolver_i->get_proxies_for_url(proxy_resolver->base, proxy_resolver->url);

    LOG_INFO("proxy_resolver 0x%p - resolved with thread_pool = %s\n", proxy_resolver->base,
             proxy_resolver_get_list(proxy_resolver) ? proxy_resolver_get_list(proxy_resolver) : "DIRECT");

    proxy_resolver->pending = false;
}

bool proxy_resolver_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;

    LOG_INFO("proxy_resolver 0x%p - getting proxies for url %s\n", proxy_resolver->base, url);

    // Call get_proxies_for_url directly since the underlying interface is asynchronous
    if (g_proxy_resolver.proxy_resolver_i->is_async()) {
        bool ok = g_proxy_resolver.proxy_resolver_i->get_proxies_for_url(ctx, url);
        LOG_INFO("proxy_resolver 0x%p - resolved = %s\n", proxy_resolver->base,
                 proxy_resolver_get_list(proxy_resolver) ? proxy_resolver_get_list(proxy_resolver) : "DIRECT");
        return ok;
    }

    free(proxy_resolver->url);
    proxy_resolver->url = strdup(url);
    proxy_resolver->pending = true;

    return threadpool_enqueue(g_proxy_resolver.thread_pool, proxy_resolver,
                              proxy_resolver_get_proxies_for_url_threadpool);
}

const char *proxy_resolver_get_list(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    char *list = NULL;
    g_proxy_resolver.proxy_resolver_i->get_list(proxy_resolver->base, &list);
    return list;
}

bool proxy_resolver_get_error(void *ctx, int32_t *error) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i) {
        *error = ENOSYS;
        return false;
    }
    return g_proxy_resolver.proxy_resolver_i->get_error(proxy_resolver->base, error);
}

bool proxy_resolver_is_pending(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    if (!g_proxy_resolver.proxy_resolver_i->is_async())
        return proxy_resolver->pending;
    return g_proxy_resolver.proxy_resolver_i->is_pending(proxy_resolver->base);
}

bool proxy_resolver_cancel(void *ctx) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    return g_proxy_resolver.proxy_resolver_i->cancel(proxy_resolver->base);
}

bool proxy_resolver_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    proxy_resolver_s *proxy_resolver = (proxy_resolver_s *)ctx;
    if (!proxy_resolver || !g_proxy_resolver.proxy_resolver_i)
        return false;
    return g_proxy_resolver.proxy_resolver_i->set_resolved_callback(proxy_resolver->base, user_data, callback);
}

void *proxy_resolver_create(void) {
    if (!g_proxy_resolver.proxy_resolver_i)
        return NULL;
    proxy_resolver_s *proxy_resolver = calloc(1, sizeof(proxy_resolver_s));
    if (!proxy_resolver)
        return NULL;
    proxy_resolver->base = g_proxy_resolver.proxy_resolver_i->create();
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
    g_proxy_resolver.proxy_resolver_i->delete (&proxy_resolver->base);
    free(proxy_resolver);
    *ctx = NULL;
    return true;
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
    if (!g_proxy_resolver.proxy_resolver_i) {
        LOG_ERROR("No proxy resolver available\n");
        return false;
    }

    // No need to create thread pool since underlying implementation is already asynchronous
    if (g_proxy_resolver.proxy_resolver_i->is_async())
        return true;

    // Create thread pool to handle proxy resolution requests asynchronously
    g_proxy_resolver.thread_pool = threadpool_create(THREADPOOL_DEFAULT_MIN_THREADS, THREADPOOL_DEFAULT_MAX_THREADS);
    if (!g_proxy_resolver.thread_pool) {
        LOG_ERROR("Failed to create thread pool\n");
        proxy_resolver_uninit();
        return false;
    }
    return true;
}

bool proxy_resolver_uninit(void) {
    if (g_proxy_resolver.thread_pool)
        threadpool_delete(&g_proxy_resolver.thread_pool);

    if (g_proxy_resolver.proxy_resolver_i)
        g_proxy_resolver.proxy_resolver_i->uninit();

    memset(&g_proxy_resolver, 0, sizeof(g_proxy_resolver));
    return false;
}
