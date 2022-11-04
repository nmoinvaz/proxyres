#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

#include "config.h"
#include "log.h"
#include "resolver.h"
#include "resolver_i.h"
#include "resolver_mac.h"

#define PROXY_RESOLVER_RUN_LOOP    CFSTR("proxy_resolver_mac.run_loop")
#define PROXY_RESOLVER_TIMEOUT_SEC 10

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

    free(proxy_resolver->list);
    proxy_resolver->list = NULL;
}

static void proxy_resolver_mac_auto_config_result_callback(void *client, CFArrayRef proxy_array, CFErrorRef error) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)client;
    if (error) {
        // Get error code
        proxy_resolver->error = CFErrorGetCode(error);
    } else {
        // Convert proxy array into PAC file return format
        int32_t proxy_count = CFArrayGetCount(proxy_array);
        int32_t max_list = proxy_count * MAX_PROXY_URL + 1;
        int32_t list_len = 0;

        proxy_resolver->list = (char *)calloc(max_list, sizeof(char));

        // Enumerate through each proxy in the array
        for (int32_t i = 0; proxy_resolver->list && i < proxy_count; i++) {
            CFDictionaryRef proxy = CFArrayGetValueAtIndex(proxy_array, i);
            CFStringRef proxy_type = (CFStringRef)CFDictionaryGetValue(proxy, kCFProxyTypeKey);

            // Copy type of connection
            if (CFEqual(proxy_type, kCFProxyTypeNone)) {
                strncat(proxy_resolver->list, "DIRECT", max_list - list_len - 1);
                list_len += 6;
            } else if (CFEqual(proxy_type, kCFProxyTypeHTTP)) {
                strncat(proxy_resolver->list, "HTTP ", max_list - list_len - 1);
                list_len += 5;
            } else if (CFEqual(proxy_type, kCFProxyTypeHTTPS)) {
                strncat(proxy_resolver->list, "HTTPS ", max_list - list_len - 1);
                list_len += 6;
            } else if (CFEqual(proxy_type, kCFProxyTypeSOCKS)) {
                strncat(proxy_resolver->list, "SOCKS ", max_list - list_len - 1);
                list_len += 6;
            } else if (CFEqual(proxy_type, kCFProxyTypeFTP)) {
                strncat(proxy_resolver->list, "FTP ", max_list - list_len - 1);
                list_len += 4;
            } else {
                LOG_WARN("Unknown proxy type encountered\n");
                continue;
            }

            if (!CFEqual(proxy_type, kCFProxyTypeNone)) {
                // Copy proxy host
                CFStringRef host = (CFStringRef)CFDictionaryGetValue(proxy, kCFProxyHostNameKey);
                if (host) {
                    const char *host_utf8 = CFStringGetCStringPtr(host, kCFStringEncodingUTF8);
                    if (host_utf8) {
                        strncat(proxy_resolver->list, host_utf8, max_list - list_len - 1);
                        list_len += strlen(host_utf8);
                    } else {
                        CFStringGetCString(host, proxy_resolver->list + list_len, max_list - list_len,
                                           kCFStringEncodingUTF8);
                        list_len = strlen(proxy_resolver->list);
                    }
                }
                // Copy proxy port
                CFNumberRef port = (CFNumberRef)CFDictionaryGetValue(proxy, kCFProxyPortNumberKey);
                if (port) {
                    int64_t port_number = 0;
                    CFNumberGetValue(port, kCFNumberSInt64Type, &port_number);
                    snprintf(proxy_resolver->list + list_len, max_list - list_len, ":%" PRId64 "", port_number);
                    list_len = strlen(proxy_resolver->list);
                }
            }

            if (i != proxy_count - 1) {
                // Append semi-colon separator
                strncat(proxy_resolver->list, ";", max_list - list_len - 1);
                list_len++;
            }
        }

        if (!proxy_resolver->list)
            proxy_resolver->error = ENOMEM;
    }

    CFRunLoopStop(CFRunLoopGetCurrent());
    return;
}

bool proxy_resolver_mac_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_mac_s *proxy_resolver = (proxy_resolver_mac_s *)ctx;
    CFURLRef target_url_ref = NULL;
    CFURLRef url_ref = NULL;
    char *auto_config_url = NULL;

    if (!proxy_resolver || !url)
        return false;

    proxy_resolver_mac_reset(proxy_resolver);

    // Prioritize proxy auto config url over manually configured proxy
    auto_config_url = proxy_config_get_auto_config_url();
    if (auto_config_url) {
        url_ref = CFURLCreateWithBytes(NULL, (const UInt8 *)auto_config_url, strlen(auto_config_url),
                                       kCFStringEncodingUTF8, NULL);

        if (!url_ref) {
            proxy_resolver->error = ENOMEM;
            LOG_ERROR("Unable to create auto config url reference (%" PRId32 ")\n", proxy_resolver->error);
            goto mac_error;
        }

        target_url_ref = CFURLCreateWithBytes(NULL, (const UInt8 *)url, strlen(url), kCFStringEncodingUTF8, NULL);
        if (!target_url_ref) {
            proxy_resolver->error = ENOMEM;
            LOG_ERROR("Unable to create target url reference (%" PRId32 ")\n", proxy_resolver->error);
            goto mac_error;
        }

        CFStreamClientContext context = {0, proxy_resolver, NULL, NULL, NULL};

        CFRunLoopSourceRef run_loop = CFNetworkExecuteProxyAutoConfigurationURL(
            url_ref, target_url_ref, proxy_resolver_mac_auto_config_result_callback, &context);

        CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop, PROXY_RESOLVER_RUN_LOOP);
        CFRunLoopRunInMode(PROXY_RESOLVER_RUN_LOOP, PROXY_RESOLVER_TIMEOUT_SEC, false);
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), run_loop, PROXY_RESOLVER_RUN_LOOP);
        goto mac_done;
    } else {
        // No proxy auto config url specified so use manually configured proxy
        char *proxy = proxy_config_get_proxy(url);
        if (proxy)
            proxy_resolver->list = proxy;
    }

    goto mac_done;

mac_error:
mac_done:

    proxy_resolver->pending = false;

    // Trigger user callback once done
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

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

bool proxy_resolver_mac_is_async(void) {
    return false;
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
                                                      proxy_resolver_mac_is_async,
                                                      proxy_resolver_mac_init,
                                                      proxy_resolver_mac_uninit};
    return &proxy_resolver_mac_i;
}
