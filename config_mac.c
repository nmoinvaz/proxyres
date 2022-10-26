#include <stdint.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

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

bool proxy_config_get_auto_config_url(char *url, int32_t max_url) {
    CFDictionaryRef proxy_settings = NULL;
    CFStringRef AutoConfigURL = NULL;
    bool has_value = false;

    if (url == NULL || max_url <= 0)
        return false;

    *url = 0;

    proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return false;

    // Check to see if auto-config url is enabled
    if (get_cf_dictionary_bool(proxy_settings, kCFNetworkProxiesProxyAutoConfigEnable) == true) {
        // Get the auto-config url
        CFStringRef auto_config_url = CFDictionaryGetValue(proxy_settings, kCFNetworkProxiesProxyAutoConfigURLString);
        if (auto_config_url != NULL) {
            CFStringGetCString(auto_config_url, url, max_url, kCFStringEncodingUTF8);
            has_value = true;
        }
    }

    CFRelease(proxy_settings);
    return has_value;
}

bool proxy_config_mac_get_proxy(char *protocol, char *proxy, int32_t max_proxy) {
    CFDictionaryRef proxy_settings = NULL;
    CFStringRef AutoConfigURL = NULL;
    bool has_value = false;

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
        return false;

    if (get_cf_dictionary_bool(proxy_settings, enable_index) == true) {
        // Get the proxy url associated with the protocol
        CFStringRef proxy_url = CFDictionaryGetValue(proxy_settings, url_index);
        if (proxy_url != NULL) {
            CFStringGetCString(proxy_url, proxy, max_proxy, kCFStringEncodingUTF8);
            has_value = true;
        }

        // Get the proxy port associated with the protocol
        CFStringRef proxy_port = CFDictionaryGetValue(proxy_settings, port_index);
        if (proxy_port != NULL) {
            int32_t proxy_len = strlen(proxy);
            // Append the proxy port to the proxy url
            strncat(proxy, ":", max_proxy - proxy_len - 1);
            proxy_len++;
            proxy[max_proxy - 1] = 0;

            CFStringGetCString(proxy_port, proxy + proxy_len, max_proxy - proxy_len, kCFStringEncodingUTF8);
        }
    }

    CFRelease(proxy_settings);
    return has_value;
}

bool proxy_config_mac_get_bypass_list(char *bypass_list, int32_t max_bypass_list) {
    CFDictionaryRef proxy_settings = NULL;
    CFStringRef AutoConfigURL = NULL;
    bool has_value = false;

    proxy_settings = CFNetworkCopySystemProxySettings();
    if (!proxy_settings)
        return false;

    // Get proxy bypass list
    CFStringRef exceptions_list = CFDictionaryGetValue(proxy_settings, kCFNetworkProxiesExceptionsList);
    if (exceptions_list != NULL) {
        CFStringGetCString(exceptions_list, bypass_list, max_bypass_list, kCFStringEncodingUTF8);
        has_value = true;
    }

    CFRelease(proxy_settings);
    return has_value;
}
