#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

#include "resolver.h"
#include "resolver_i.h"
#include "resolver_mac.h"

#include "config.h"

typedef struct g_proxy_resolver_mac_s {
    bool reserved;
} g_proxy_resolver_win2k_s;

struct g_proxy_resolver_mac_s g_proxy_resolver_mac;

typedef struct proxy_resolver_mac_s {
    // Last system error
    int32_t error;
    // Resolved user callback
    void *user_data;
    proxy_resolver_resolved_cb callback;
    // Resolution pending
    bool pending;
    // Proxy list
    char *list;
} proxy_resolver_mac_s;

static void proxy_resolver_mac_reset(proxy_resolver_mac_s *proxy_resolver) {
    proxy_resolver->pending = false;
    proxy_resolver->error = 0;
    if (proxy_resolver->list) {
        free(proxy_resolver->list);
        proxy_resolver->list = NULL;
    }
}

static void proxy_resolver_mac_auto_config_result_callback(CFUnsafeMutableRawPointer client, CFArray proxy_list,
                                                           CFError error) {
    proxy_resolver_mac_s *proxy_resolver;
    CFStreamClientContext *context = (CFStreamClientContext *)client;
    if (!context)
        return;
    proxy_resolver = (proxy_resolver_mac_s *)context->info;
    if (!proxy_resolver)
        return;

    return;
}

bool proxy_resolver_mac_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)ctx;
    CFURLRef target_url_ref = NULL;
    CFURLRef url_ref = NULL;
    char *proxy = NULL;
    char *auto_config_url = NULL;
    bool is_ok = false;

    if (!proxy_resolver || !url)
        return false;

    proxy_resolver_mac_reset(proxy_resolver);

    proxy = proxy_config_get_proxy(url);
    if (proxy != NULL) {
        proxy_resolver->list = proxy;
        goto mac_done;
    }

    target_url_ref = CFURLCreateWithBytes(NULL, (const UInt8 *)url, strlen(url), kCFStringEncodingUTF8, NULL);
    if (target_url_ref == NULL) {
        proxy_resolver->error = ENOMEM;
        printf("Unable to create target url reference (%d)\n", proxy_resolver->error);
        goto mac_error;
    }

    auto_config_url = proxy_config_get_auto_config_url();
    if (auto_config_url != NULL) {
        url_ref = CFURLCreateWithBytes(NULL, (const UInt8 *)auto_config_url, strlen(auto_config_url),
                                       kCFStringEncodingUTF8, NULL);

        if (url_ref == NULL) {
            proxy_resolver->error = ENOMEM;
            printf("Unable to create auto config url reference (%d)\n", proxy_resolver->error);
            goto mac_error;
        }

        CFStreamClientContext context = {0, proxy_resolver, NULL, NULL, NULL};
        CFNetworkExecuteProxyAutoConfigurationURL(url_ref, target_url_ref,
                                                  proxy_resolver_mac_auto_config_result_callback, &context);
    }

    goto mac_cleanup;

mac_error:
mac_done:

    proxy_resolver->pending = false;

    // Trigger user callback once done
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

mac_cleanup:
    free(proxy);
    free(auto_config_url);

    if (url_ref)
        CFRelease(url_ref);
    if (target_url_ref)
        CFRelease(target_url_ref);

    return proxy_resolver->error == 0;
}

bool proxy_resolver_mac_get_list(void *ctx, char **list) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)ctx;
    if (!proxy_resolver || !list)
        return false;
    *list = proxy_resolver->list;
    return (*list != NULL);
}

bool proxy_resolver_mac_get_error(void *ctx, int32_t *error) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)ctx;
    if (!proxy_resolver || !error)
        return false;
    *error = proxy_resolver->error;
    return true;
}

bool proxy_resolver_mac_is_pending(void *ctx) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)ctx;
    if (!proxy_resolver)
        return false;
    return proxy_resolver->pending;
}

bool proxy_resolver_mac_is_blocking(void *ctx) {
    return true;
}

bool proxy_resolver_mac_cancel(void *ctx) {
    return false;
}

bool proxy_resolver_mac_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver->user_data = user_data;
    proxy_resolver->callback = callback;
    return true;
}

void *proxy_resolver_mac_create(void) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)calloc(1, sizeof(proxy_resolver_mac_s));
    return proxy_resolver;
}

bool proxy_resolver_mac_delete(void **ctx) {
    proxy_resolver_mac_s *proxy_resolver;
    if (ctx == NULL)
        return false;
    proxy_resolver = (proxy_resolver_mac_s *)*ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver_mac_cancel(ctx);
    free(proxy_resolver);
    return true;
}

bool proxy_resolver_mac_init(void) {
    return true;
}

bool proxy_resolver_mac_uninit(void) {
    memset(&g_proxy_resolver_mac, 0, sizeof(g_proxy_resolver_mac));
    return true;
}

proxy_resolver_i_s *proxy_resolver_mac_get_interface(void) {
    static proxy_resolver_i_s proxy_resolver_mac_i = {proxy_resolver_mac_get_proxies_for_url,
                                                      proxy_resolver_mac_get_list,
                                                      proxy_resolver_mac_get_error,
                                                      proxy_resolver_mac_is_pending,
                                                      proxy_resolver_mac_cancel,
                                                      proxy_resolver_mac_set_resolved_callback,
                                                      proxy_resolver_mac_create,
                                                      proxy_resolver_mac_delete,
                                                      proxy_resolver_mac_is_blocking,
                                                      proxy_resolver_mac_init,
                                                      proxy_resolver_mac_uninit};
    return &proxy_resolver_mac_i;
}
