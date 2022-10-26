#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#if defined(__APPLE__)
#  include "config_mac.h"
#elif defined(__linux__)
#  include "config_gnome3.h"
#elif defined(_WIN32)
#  include "config_win.h"
#endif

bool proxy_config_get_auto_discover(void) {
#if defined(__APPLE__)
    return proxy_config_mac_get_auto_discover();
#elif defined(__linux__)
    return proxy_config_linux_get_auto_discover();
#elif defined(_WIN32)
    return proxy_config_win_get_auto_discover();
#endif
}

bool proxy_config_get_auto_config_url(char *url, int32_t max_url) {
#if defined(__APPLE__)
    return proxy_config_get_auto_config_url(url, max_url);
#elif defined(__linux__)
    return proxy_config_get_auto_config_url(url, max_url);
#elif defined(_WIN32)
    return proxy_config_get_auto_config_url(url, max_url);
#endif
}

bool proxy_config_get_proxy(char *protocol, char *proxy, int32_t max_proxy) {
#if defined(__APPLE__)
    return proxy_config_mac_get_proxy(protocol, proxy, max_proxy);
#elif defined(__linux__)
    return proxy_config_linux_get_proxy(protocol, proxy, max_proxy);
#elif defined(_WIN32)
    return proxy_config_win_get_proxy(protocol, proxy, max_proxy);
#endif
}

bool proxy_config_get_bypass_list(char *bypass_list, int32_t max_bypass_list) {
#if defined(__APPLE__)
    return proxy_config_mac_get_bypass_list(bypass_list, max_bypass_list);
#elif defined(__linux__)
    return proxy_config_linux_get_bypass_list(bypass_list, max_bypass_list);
#elif defined(_WIN32)
    return proxy_config_win_get_bypass_list(bypass_list, max_bypass_list);
#endif
}
