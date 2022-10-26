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

const char *proxy_config_get_auto_config_url(void) {
#if defined(__APPLE__)
    return proxy_config_get_auto_config_url();
#elif defined(__linux__)
    return proxy_config_get_auto_config_url();
#elif defined(_WIN32)
    return proxy_config_get_auto_config_url();
#endif
}

const char *proxy_config_get_proxy(const char *protocol) {
#if defined(__APPLE__)
    return proxy_config_mac_get_proxy(protocol);
#elif defined(__linux__)
    return proxy_config_linux_get_proxy(protocol);
#elif defined(_WIN32)
    return proxy_config_win_get_proxy(protocol);
#endif
}

const char *proxy_config_get_bypass_list(void) {
#if defined(__APPLE__)
    return proxy_config_mac_get_bypass_list();
#elif defined(__linux__)
    return proxy_config_linux_get_bypass_list();
#elif defined(_WIN32)
    return proxy_config_win_get_bypass_list();
#endif
}
