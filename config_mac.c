#include <stdint.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

#include "config.h"
#include "config_i.h"
#include "config_mac.h"

static bool get_cf_dictionary_bool(CFDictionaryRef dictionary, CFStringRef key) {
    CFNumberRef item = NULL;
    int value = 0;

    return CFDictionaryGetValueIfPresent(dictionary, key, (const void **)&item) &&
        CFNumberGetValue(item, kCFNumberIntType, &value) && value;
}

bool proxy_config_mac_get_auto_discover(void) {
    CFDictionaryRef proxy_settings = NULL;
    CFStringRef AutoConfigURL = NULL;
    bool auto_discover = false;

    proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return false;

    // Get whether or not auto discovery is enabled
    if (get_cf_dictionary_bool(proxy_settings, kCFNetworkProxiesProxyAutoDiscoveryEnable) == true)
        auto_discover = true;

    CFRelease(proxy_settings);
    return auto_discover;
}

char *proxy_config_get_auto_config_url(void) {
    CFDictionaryRef proxy_settings = NULL;
    CFStringRef AutoConfigURL = NULL;
    const char *url = NULL;

    proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return NULL;

    // Check to see if auto-config url is enabled
    if (get_cf_dictionary_bool(proxy_settings, kCFNetworkProxiesProxyAutoConfigEnable) == true) {
        // Get the auto-config url
        CFStringRef auto_config_url = CFDictionaryGetValue(proxy_settings, kCFNetworkProxiesProxyAutoConfigURLString);
        if (auto_config_url != NULL) {
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
    CFDictionaryRef proxy_settings = NULL;
    CFStringRef AutoConfigURL = NULL;
    char *proxy = NULL;
    int32_t max_proxy = 0;

    // Determine the indexes to retrieve from the system proxy settings to get the value
    // for the proxy list we want
    int32_t enable_index = kCFNetworkProxiesHTTPEnable;
    int32_t url_index = kCFNetworkProxiesHTTPProxy;
    int32_t port_index = kCFNetworkProxiesHTTPPort;

    if (strcmp(protocol, "https") == 0) {
        enable_index = kCFNetworkProxiesHTTPSEnable;
        url_index = kCFNetworkProxiesHTTPSProxy;
        port_index = kCFNetworkProxiesHTTPSPort;
    }

    proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return NULL;

    if (get_cf_dictionary_bool(proxy_settings, enable_index) == true) {
        // Get the proxy url associated with the protocol
        CFStringRef proxy_url = CFDictionaryGetValue(proxy_settings, url_index);
        if (proxy_url != NULL) {
            const char *proxy_url_p = CFStringGetCStringPtr(proxy_url, proxy, max_proxy, kCFStringEncodingUTF8);
            if (proxy_url_p) {
                max_proxy = strlen(proxy_url_p) + 32; // Allow enough room for port number
                proxy = calloc(max_proxy, sizeof(char));
                strncat(proxy, proxy_url_p, max_proxy);
            }
        }

        // Get the proxy port associated with the protocol
        CFStringRef proxy_port = CFDictionaryGetValue(proxy_settings, port_index);
        if (proxy_port != NULL) {
            // Append the proxy port to the proxy url
            int32_t proxy_len = strlen(proxy);
            strncat(proxy + proxy_len, ":", max_proxy - proxy_len - 1);
            proxy_len++;
            proxy[max_proxy - 1] = 0;

            CFStringGetCString(proxy_port, proxy + proxy_len, max_proxy - proxy_len, kCFStringEncodingUTF8);
        }
    }

    CFRelease(proxy_settings);
    return proxy;
}

char *proxy_config_mac_get_bypass_list(void) {
    CFDictionaryRef proxy_settings = NULL;
    CFStringRef AutoConfigURL = NULL;
    const char *bypass_list = NULL;

    proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return NULL;

    // Get proxy bypass list
    CFStringRef exceptions_list = CFDictionaryGetValue(proxy_settings, kCFNetworkProxiesExceptionsList);
    if (exceptions_list != NULL) {
        const char *exceptions_list_p = CFStringGetCStringPtr(exceptions_list, kCFStringEncodingUTF8);
        if (exceptions_list_p) {
            bypass_list = strdup(exceptions_list_p);
        }
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
