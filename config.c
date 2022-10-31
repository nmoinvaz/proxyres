#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "log.h"
#include "config.h"
#include "config_i.h"
#include "config_env.h"
#if defined(__APPLE__)
#include "config_mac.h"
#elif defined(__linux__)
#include "config_gnome2.h"
#include "config_gnome3.h"
#include "config_kde.h"
#elif defined(_WIN32)
#include "config_win.h"
#endif
#include "util_linux.h"

typedef struct g_proxy_config_s {
    // Proxy config interface
    proxy_config_i_s *proxy_config_i;
} g_proxy_config_s;

g_proxy_config_s g_proxy_config;

bool proxy_config_get_auto_discover(void) {
    if (g_proxy_config.proxy_config_i)
        return g_proxy_config.proxy_config_i->auto_discover();
    return false;
}

char *proxy_config_get_auto_config_url(void) {
    if (!g_proxy_config.proxy_config_i)
        return NULL;
    return g_proxy_config.proxy_config_i->get_auto_config_url();
}

char *proxy_config_get_proxy(const char *protocol) {
    if (!g_proxy_config.proxy_config_i)
        return NULL;
    return g_proxy_config.proxy_config_i->get_proxy(protocol);
}

char *proxy_config_get_bypass_list(void) {
    if (!g_proxy_config.proxy_config_i)
        return NULL;
    return g_proxy_config.proxy_config_i->get_bypass_list();
}

bool proxy_config_init(void) {
#if defined(__APPLE__)
    if (proxy_config_mac_init())
        g_proxy_config.proxy_config_i = proxy_config_mac_get_interface();
#elif defined(__linux__)
    int32_t desktop_env = get_desktop_env();

    switch (desktop_env) {
    case DESKTOP_ENV_GNOME3:
        if (proxy_config_gnome3_init())
            g_proxy_config.proxy_config_i = proxy_config_gnome3_get_interface();
        break;
    case DESKTOP_ENV_GNOME2:
        if (proxy_config_gnome2_init())
            g_proxy_config.proxy_config_i = proxy_config_gnome2_get_interface();
        break;
    case DESKTOP_ENV_KDE5:
    case DESKTOP_ENV_KDE4:
    case DESKTOP_ENV_KDE3:
        if (proxy_config_kde_init())
            g_proxy_config.proxy_config_i = proxy_config_kde_get_interface();
        break;
    }

    if (!g_proxy_config.proxy_config_i)
        g_proxy_config.proxy_config_i = proxy_config_env_get_interface();
#elif defined(_WIN32)
    if (proxy_config_win_init())
        g_proxy_config.proxy_config_i = proxy_config_win_get_interface();
#endif
    if (!g_proxy_config.proxy_config_i) {
        LOG_ERROR("No config interface found\n");
        return false;
    }
    return true;
}

bool proxy_config_uninit(void) {
    if (g_proxy_config.proxy_config_i)
        return g_proxy_config.proxy_config_i->uninit();
    return false;
}
