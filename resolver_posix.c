#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <errno.h>

#include "config.h"
#include "fetch.h"
#include "log.h"
#include "execute.h"
#include "resolver.h"
#include "resolver_i.h"
#include "resolver_posix.h"
#include "util.h"
#include "wpad_dhcp.h"
#include "wpad_dns.h"

#define WPAD_DHCP_TIMEOUT (10)

typedef struct g_proxy_resolver_posix_s {
    // Auto-config script
    char *script;
} g_proxy_resolver_posix_s;

g_proxy_resolver_posix_s g_proxy_resolver_posix;

typedef struct proxy_resolver_posix_s {
    // Last system error
    int32_t error;
    // Resolution pending
    bool pending;
    // Proxy list
    char *list;
} proxy_resolver_posix_s;

static void proxy_resolver_posix_cleanup(proxy_resolver_posix_s *proxy_resolver) {
    free(proxy_resolver->list);
    proxy_resolver->list = NULL;
}

static void proxy_resolver_posix_reset(proxy_resolver_posix_s *proxy_resolver) {
    proxy_resolver->pending = false;
    proxy_resolver->error = 0;

    proxy_resolver_posix_cleanup(proxy_resolver);
}

bool proxy_resolver_posix_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)ctx;
    char **proxies = NULL;
    char *proxy = NULL;
    char *script = NULL;
    const char *auto_config_url = NULL;

    proxy_resolver_posix_reset(proxy_resolver);

    if (proxy_config_get_auto_discover()) {
        // Detect proxy auto configuration using DHCP
        auto_config_url = wpad_dhcp(WPAD_DHCP_TIMEOUT);
        if (!auto_config_url) {
            // Detect proxy auto configuration using DNS
            script = wpad_dns(NULL);
        }
    }

    // Use manually specified proxy auto configuration
    if (!auto_config_url && !script)
        auto_config_url = proxy_config_get_auto_config_url();

    if (auto_config_url || script) {
        // Download proxy auto-config script if found
        if (script) {
            free(g_proxy_resolver_posix.script);
            g_proxy_resolver_posix.script = script;
        } else if (!g_proxy_resolver_posix.script) {
            g_proxy_resolver_posix.script = fetch_get(auto_config_url, &proxy_resolver->error);
            if (!g_proxy_resolver_posix.script) {
                LOG_ERROR("Unable to download auto-config script (%" PRId32 ")\n", proxy_resolver->error);
                return false;
            }
        }

        // Execute blocking proxy auto config script for url
        void *proxy_execute = proxy_execute_create();
        if (!proxy_execute) {
            proxy_resolver->error = ENOMEM;
            LOG_ERROR("Unable to create proxy execute object (%" PRId32 ")\n", proxy_resolver->error);
            return false;
        }

        if (!proxy_execute_get_proxies_for_url(proxy_execute, g_proxy_resolver_posix.script, url)) {
            proxy_execute_get_error(proxy_execute, &proxy_resolver->error);
            LOG_ERROR("Unable to get proxies for url (%" PRId32 ")\n", proxy_resolver->error);
            return false;
        }

        const char *list = proxy_execute_get_list(proxy_execute);
        proxy_resolver->list = convert_proxy_list_to_uri_list(list);

        proxy_execute_delete(proxy_execute);
    } else if ((proxy = proxy_config_get_proxy(url)) != NULL) {
        // Use explicit proxy list
        proxy_resolver->list = proxy;
    } else {
        // Use DIRECT connection
        free(proxy_resolver->list);
        proxy_resolver->list = NULL;
    }

    return true;
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

bool proxy_resolver_posix_is_pending(void *ctx) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)ctx;
    if (!proxy_resolver)
        return false;
    return proxy_resolver->pending;
}

bool proxy_resolver_posix_cancel(void *ctx) {
    return false;
}

bool proxy_resolver_posix_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    proxy_resolver_posix_s *proxy_resolver = (proxy_resolver_posix_s *)ctx;
    if (!proxy_resolver)
        return false;
    //proxy_resolver->user_data = user_data;
    //proxy_resolver->callback = callback;
    return true;
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
    free(proxy_resolver);
    return true;
}

bool proxy_resolver_posix_is_async(void) {
    return false;
}

bool proxy_resolver_posix_init(void) {
    if (!fetch_init())
        return false;
    return proxy_execute_init();
}

bool proxy_resolver_posix_uninit(void) {
    fetch_uninit();

    if (g_proxy_resolver_posix.script)
        free(g_proxy_resolver_posix.script);

    proxy_execute_uninit();

    memset(&g_proxy_resolver_posix, 0, sizeof(g_proxy_resolver_posix));
    return true;
}

proxy_resolver_i_s *proxy_resolver_posix_get_interface(void) {
    static proxy_resolver_i_s proxy_resolver_posix_i = {proxy_resolver_posix_get_proxies_for_url,
                                                         proxy_resolver_posix_get_list,
                                                         proxy_resolver_posix_get_error,
                                                         proxy_resolver_posix_is_pending,
                                                         proxy_resolver_posix_cancel,
                                                         proxy_resolver_posix_set_resolved_callback,
                                                         proxy_resolver_posix_create,
                                                         proxy_resolver_posix_delete,
                                                         proxy_resolver_posix_is_async,
                                                         proxy_resolver_posix_init,
                                                         proxy_resolver_posix_uninit};
    return &proxy_resolver_posix_i;
}
