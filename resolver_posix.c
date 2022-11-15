#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>

#include "config.h"
#include "fetch.h"
#include "log.h"
#include "execute.h"
#include "mutex.h"
#include "net_adapter.h"
#include "resolver.h"
#include "resolver_i.h"
#include "resolver_posix.h"
#include "signal.h"
#include "threadpool.h"
#include "util.h"
#include "wpad_dhcp.h"
#include "wpad_dns.h"

#define WPAD_DHCP_TIMEOUT   (3)
#define WPAD_EXPIRE_SECONDS (300)

typedef struct g_proxy_resolver_posix_s {
    // WPAD discovered url
    char *auto_config_url;
    // WPAD discovery lock
    void *mutex;
    // PAC script
    char *script;
    time_t last_wpad_time;
    time_t last_fetch_time;
} g_proxy_resolver_posix_s;

g_proxy_resolver_posix_s g_proxy_resolver_posix;

typedef struct proxy_resolver_posix_s {
    // Last system error
    int32_t error;
    // Complete signal
    void *complete;
    // Proxy list
    char *list;
} proxy_resolver_posix_s;

static void proxy_resolver_posix_cleanup(proxy_resolver_posix_s *proxy_resolver) {
    free(proxy_resolver->list);
    proxy_resolver->list = NULL;
}

static void proxy_resolver_posix_reset(proxy_resolver_posix_s *proxy_resolver) {
    proxy_resolver->error = 0;

    signal_delete(&proxy_resolver->complete);
    proxy_resolver->complete = signal_create();

    proxy_resolver_posix_cleanup(proxy_resolver);
}

static char *proxy_resolver_posix_wpad_discover(void) {
    char *auto_config_url = NULL;
    char *script = NULL;

    // Check if we need to re-discover the WPAD auto config url
    if (g_proxy_resolver_posix.last_wpad_time > 0 &&
        g_proxy_resolver_posix.last_wpad_time + WPAD_EXPIRE_SECONDS >= time(NULL)) {
        // Use cached version of WPAD auto config url
        auto_config_url = g_proxy_resolver_posix.auto_config_url;
    } else {
        free(g_proxy_resolver_posix.auto_config_url);
        g_proxy_resolver_posix.auto_config_url = NULL;

        // Detect proxy auto configuration using DHCP
        LOG_INFO("Discovering proxy auto config using WPAD (%s)\n", "DHCP");
        auto_config_url = wpad_dhcp(WPAD_DHCP_TIMEOUT);

        // Detect proxy auto configuration using DNS
        if (!auto_config_url) {
            LOG_INFO("Discovering proxy auto config using WPAD (%s)\n", "DNS");
            script = wpad_dns(NULL);
            if (script) {
                g_proxy_resolver_posix.script = script;
                g_proxy_resolver_posix.last_fetch_time = time(NULL);
            }
        }

        g_proxy_resolver_posix.auto_config_url = auto_config_url;
        g_proxy_resolver_posix.last_wpad_time = time(NULL);
    }

    // Duplicate so it can be freed the same if proxy_config_get_auto_config_url() returns a string
    return auto_config_url ? strdup(auto_config_url) : NULL;
}

static char *proxy_resolver_posix_fetch_pac(const char *auto_config_url, int32_t *error) {
    char *script = NULL;

    // Check if the auto config url has changed
    bool url_changed = false;
    if (g_proxy_resolver_posix.auto_config_url)
        url_changed = strcmp(g_proxy_resolver_posix.auto_config_url, auto_config_url) != 0;
    else
        url_changed = true;

    // Check if we need to re-fetch the PAC script
    if (g_proxy_resolver_posix.last_fetch_time > 0 &&
        g_proxy_resolver_posix.last_fetch_time + WPAD_EXPIRE_SECONDS >= time(NULL) && !url_changed) {
        // Use cached version of the PAC script
        script = g_proxy_resolver_posix.script;
    } else {
        LOG_INFO("Fetching proxy auto config script from %s\n", auto_config_url);

        script = fetch_get(auto_config_url, error);
        if (!script)
            LOG_ERROR("Unable to fetch proxy auto config script %s (%" PRId32 ")\n", auto_config_url, *error);

        free(g_proxy_resolver_posix.script);
        g_proxy_resolver_posix.script = script;
        g_proxy_resolver_posix.last_fetch_time = time(NULL);
    }

    return script;
}

bool proxy_resolver_posix_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)ctx;
    char **proxies = NULL;
    char *proxy = NULL;
    char *script = NULL;
    char *auto_config_url = NULL;
    bool locked = false;
    bool is_ok = false;

    proxy_resolver_posix_reset(proxy_resolver);

    if (proxy_config_get_auto_discover()) {
        locked = mutex_lock(g_proxy_resolver_posix.mutex);
        // Discover the proxy auto config url
        auto_config_url = proxy_resolver_posix_wpad_discover();
    }

    // Use manually specified proxy auto configuration
    if (!auto_config_url)
        auto_config_url = proxy_config_get_auto_config_url();

    if (auto_config_url) {
        // Download proxy auto config script if available
        script = proxy_resolver_posix_fetch_pac(auto_config_url, &proxy_resolver->error);
        locked = !locked || !mutex_unlock(g_proxy_resolver_posix.mutex);

        if (!script)
            goto posix_cleanup;

        // Execute blocking proxy auto config script for url
        void *proxy_execute = proxy_execute_create();
        if (!proxy_execute) {
            proxy_resolver->error = ENOMEM;
            LOG_ERROR("Unable to create proxy execute object (%" PRId32 ")\n", proxy_resolver->error);
            goto posix_cleanup;
        }

        if (!proxy_execute_get_proxies_for_url(proxy_execute, g_proxy_resolver_posix.script, url)) {
            proxy_execute_get_error(proxy_execute, &proxy_resolver->error);
            LOG_ERROR("Unable to get proxies for url (%" PRId32 ")\n", proxy_resolver->error);
            goto posix_cleanup;
        }

        // Get return value from FindProxyForURL
        const char *list = proxy_execute_get_list(proxy_execute);

        // When PROXY is returned then use scheme associated with the URL
        char *scheme = get_url_scheme(url, "http");

        // Convert return value from FindProxyForURL to uri list
        proxy_resolver->list = convert_proxy_list_to_uri_list(list, scheme);
        free(scheme);

        proxy_execute_delete(proxy_execute);
    } else if ((proxy = proxy_config_get_proxy(url)) != NULL) {
        // Use explicit proxy list
        proxy_resolver->list = proxy;
    } else {
        // Use DIRECT connection
        free(proxy_resolver->list);
        proxy_resolver->list = NULL;
    }

    if (locked)
        mutex_unlock(g_proxy_resolver_posix.mutex);

    signal_set(proxy_resolver->complete);
    is_ok = true;

posix_cleanup:
    free(auto_config_url);

    return is_ok;
}

const char *proxy_resolver_posix_get_list(void *ctx) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)ctx;
    if (!proxy_resolver)
        return NULL;
    return proxy_resolver->list;
}

bool proxy_resolver_posix_get_error(void *ctx, int32_t *error) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)ctx;
    if (!proxy_resolver || !error)
        return false;
    *error = proxy_resolver->error;
    return true;
}

bool proxy_resolver_posix_wait(void *ctx, int32_t timeout_ms) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)ctx;
    if (!proxy_resolver)
        return false;
    return signal_wait(proxy_resolver->complete, timeout_ms);
}

bool proxy_resolver_posix_cancel(void *ctx) {
    return false;
}

void *proxy_resolver_posix_create(void) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)calloc(1, sizeof(proxy_resolver_posix_s));
    return proxy_resolver;
}

bool proxy_resolver_posix_delete(void **ctx) {
    proxy_resolver_posix_s *proxy_resolver;
    if (ctx == NULL)
        return false;
    proxy_resolver = (proxy_resolver_posix_s *)*ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver_cancel(ctx);
    proxy_resolver_posix_cleanup(proxy_resolver);
    signal_delete(&proxy_resolver->complete);
    free(proxy_resolver);
    return true;
}

bool proxy_resolver_posix_is_async(void) {
    return false;
}

static void proxy_resolver_posix_wpad_startup(void *arg) {
    mutex_lock(g_proxy_resolver_posix.mutex);

    // Discover the proxy auto config url
    char *auto_config_url = proxy_resolver_posix_wpad_discover();
    if (auto_config_url) {
        int32_t error = 0;

        // Download proxy auto config script if available
        proxy_resolver_posix_fetch_pac(auto_config_url, &error);
        free(auto_config_url);
    }

    mutex_unlock(g_proxy_resolver_posix.mutex);
}

bool proxy_resolver_posix_init(void) {
    return proxy_resolver_posix_init_ex(NULL);
}

bool proxy_resolver_posix_init_ex(void *threadpool) {
    g_proxy_resolver_posix.mutex = mutex_create();
    if (!g_proxy_resolver_posix.mutex)
        return false;

    if (!fetch_init() || !proxy_execute_init())
        return proxy_resolver_posix_uninit();

    // Start WPAD discovery process immediately
    if (threadpool && proxy_config_get_auto_discover())
        threadpool_enqueue(threadpool, NULL, proxy_resolver_posix_wpad_startup);

    return true;
}

bool proxy_resolver_posix_uninit(void) {
    free(g_proxy_resolver_posix.script);
    free(g_proxy_resolver_posix.auto_config_url);
    mutex_delete(&g_proxy_resolver_posix.mutex);

    fetch_uninit();
    proxy_execute_uninit();

    memset(&g_proxy_resolver_posix, 0, sizeof(g_proxy_resolver_posix));
    return true;
}

proxy_resolver_i_s *proxy_resolver_posix_get_interface(void) {
    static proxy_resolver_i_s proxy_resolver_posix_i = {proxy_resolver_posix_get_proxies_for_url,
                                                        proxy_resolver_posix_get_list,
                                                        proxy_resolver_posix_get_error,
                                                        proxy_resolver_posix_wait,
                                                        proxy_resolver_posix_cancel,
                                                        proxy_resolver_posix_create,
                                                        proxy_resolver_posix_delete,
                                                        proxy_resolver_posix_is_async,
                                                        proxy_resolver_posix_init,
                                                        proxy_resolver_posix_uninit};
    return &proxy_resolver_posix_i;
}
