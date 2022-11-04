#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

#include "config.h"
#include "config_i.h"
#include "config_mac.h"
#include "proxyres.h"

static bool get_cf_dictionary_bool(CFDictionaryRef dictionary, CFStringRef key) {
    CFNumberRef item = NULL;
    int value = 0;

    return CFDictionaryGetValueIfPresent(dictionary, key, (const void **)&item) &&
        CFNumberGetValue(item, kCFNumberIntType, &value) && value;
}

bool proxy_config_mac_get_auto_discover(void) {
    bool auto_discover = false;

    CFDictionaryRef proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return false;

    // Get whether or not auto discovery is enabled
    if (get_cf_dictionary_bool(proxy_settings, kCFNetworkProxiesProxyAutoDiscoveryEnable) == true)
        auto_discover = true;

    CFRelease(proxy_settings);
    return auto_discover;
}

char *proxy_config_mac_get_auto_config_url(void) {
    char *url = NULL;

    CFDictionaryRef proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return NULL;

    // Check to see if auto-config url is enabled
    if (get_cf_dictionary_bool(proxy_settings, kCFNetworkProxiesProxyAutoConfigEnable) == true) {
        // Get the auto-config url
        CFStringRef auto_config_url = CFDictionaryGetValue(proxy_settings, kCFNetworkProxiesProxyAutoConfigURLString);
        if (auto_config_url) {
            const char *auto_config_url_p = CFStringGetCStringPtr(auto_config_url, kCFStringEncodingUTF8);
            if (auto_config_url_p) {
                url = strdup(auto_config_url_p);
            }
        }
    }

    CFRelease(proxy_settings);
    return url;
}

char *proxy_config_mac_get_proxy(const char *protocol) {
    char *proxy = NULL;
    int32_t max_proxy = 0;

    // Determine the indexes to retrieve from the system proxy settings to get the value
    // for the proxy list we want
    CFStringRef enable_index = kCFNetworkProxiesHTTPEnable;
    CFStringRef host_index = kCFNetworkProxiesHTTPProxy;
    CFStringRef port_index = kCFNetworkProxiesHTTPPort;

    if (strcasecmp(protocol, "https") == 0) {
        enable_index = kCFNetworkProxiesHTTPSEnable;
        host_index = kCFNetworkProxiesHTTPSProxy;
        port_index = kCFNetworkProxiesHTTPSPort;
    } else if (strcasecmp(protocol, "socks") == 0) {
        enable_index = kCFNetworkProxiesSOCKSEnable;
        host_index = kCFNetworkProxiesSOCKSProxy;
        port_index = kCFNetworkProxiesSOCKSPort;
    } else if (strcasecmp(protocol, "ftp") == 0) {
        enable_index = kCFNetworkProxiesFTPEnable;
        host_index = kCFNetworkProxiesFTPProxy;
        port_index = kCFNetworkProxiesFTPPort;
    }

    CFDictionaryRef proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return NULL;

    if (get_cf_dictionary_bool(proxy_settings, enable_index) == true) {
        // Get the proxy url associated with the protocol
        CFStringRef host = CFDictionaryGetValue(proxy_settings, host_index);
        if (host) {
            const char *host_p = CFStringGetCStringPtr(host, kCFStringEncodingUTF8);
            if (host_p) {
                max_proxy = strlen(host_p) + 32;  // Allow enough room for port number
                proxy = calloc(max_proxy, sizeof(char));
                strncat(proxy, host_p, max_proxy);
                proxy[max_proxy - 1] = 0;
            }
        }

        // Get the proxy port associated with the protocol
        CFNumberRef port = CFDictionaryGetValue(proxy_settings, port_index);
        if (proxy && port) {
            // Append the proxy port to the proxy url
            int64_t port_number = 0;
            CFNumberGetValue(port, kCFNumberSInt64Type, &port_number);
            int32_t proxy_len = strlen(proxy);
            snprintf(proxy + proxy_len, max_proxy - proxy_len, ":%" PRId64 "", port_number);
        }
    }

    CFRelease(proxy_settings);
    return proxy;
}

char *proxy_config_mac_get_bypass_list(void) {
    char *bypass_list = NULL;

    CFDictionaryRef proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return NULL;

    // Get proxy bypass list
    CFArrayRef exceptions_list = CFDictionaryGetValue(proxy_settings, kCFNetworkProxiesExceptionsList);
    if (exceptions_list) {
        // Allocate memory to copy exception list
        int32_t exception_count = CFArrayGetCount(exceptions_list);
        int32_t max_bypass_list = exception_count * MAX_PROXY_URL + 1;
        int32_t bypass_list_len = 0;

        bypass_list = (char *)calloc(max_bypass_list, sizeof(char));

        // Enumerate exception array and copy to semi-colon delimited string
        for (int32_t i = 0; bypass_list && i < exception_count; ++i) {
            CFStringRef exception = CFArrayGetValueAtIndex(exceptions_list, i);
            if (exception) {
                const char *exception_utf8 = CFStringGetCStringPtr(exception, kCFStringEncodingUTF8);
                if (exception_utf8) {
                    snprintf(bypass_list + bypass_list_len, max_bypass_list - bypass_list_len, "%s,", exception_utf8);
                    bypass_list_len += strlen(exception_utf8) + 1;
                }
            }
        }

        // Remove last separator
        if (bypass_list)
            str_trim_end(bypass_list, ',');
    }

    CFRelease(proxy_settings);
    return bypass_list;
}

bool proxy_config_mac_init(void) {
    return true;
}

bool proxy_config_mac_uninit(void) {
    return true;
}

proxy_config_i_s *proxy_config_mac_get_interface(void) {
    static proxy_config_i_s proxy_config_mac_i = {proxy_config_mac_get_auto_discover,
                                                  proxy_config_mac_get_auto_config_url,
                                                  proxy_config_mac_get_proxy,
                                                  proxy_config_mac_get_bypass_list,
                                                  proxy_config_mac_init,
                                                  proxy_config_mac_uninit};
    return &proxy_config_mac_i;
}
