#include <stdint.h>
#include <stdbool.h>

#include <winhttp.h>

#include "utils_win.h"

static void free_winhttp_ie_proxy_config(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *ie_config) {
    if (ie_config->lpszAutoConfigUrl)
        GlobalFree(ie_config->lpszAutoConfigUrl);
    if (ie_config->lpszProxy)
        GlobalFree(ie_config->lpszProxy);
    if (ie_config->lpszProxyBypass)
        GlobalFree(ie_config->lpszProxyBypass);
}

bool proxy_config_win_get_auto_discover(void) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return false;

    bool auto_discover = ie_config.fAutoDetect;
    free_winhttp_ie_proxy_config(&ie_config);
    return auto_discover;
}

bool proxy_config_win_get_auto_config_url(char *url, int32_t max_url) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    bool has_value = false;

    if (url == NULL || max_url <= 0)
        return false;

    *url = 0;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return false;

    char *auto_config_url = utf8strdup(ie_config.lpszAutoConfigUrl);
    if (auto_config_url) {
        strncpy(url, auto_config_url, max_url);
        url[max_url - 1] = 0;
        free(auto_config_url);
        has_value = true;
    }

    free_winhttp_ie_proxy_config(&ie_config);
    return has_value;
}

bool proxy_config_win_get_proxy(char *protocol, char *proxy, int32_t max_proxy) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    bool has_value = false;

    if (proxy == NULL || max_proxy <= 0)
        return false;

    *proxy = 0;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return false;

    char *list = utf8strdup(ie_config.lpszProxy);
    if (list) {
        strncpy(proxy, list, max_proxy);
        proxy[max_proxy - 1] = 0;
        free(list);
        has_value = true;
    }

    free_winhttp_ie_proxy_config(&ie_config);
    return has_value;
}

bool proxy_config_win_get_bypass_list(char *bypass_list, int32_t max_bypass_list) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    bool has_value = false;

    if (bypass_list == NULL || max_bypass_list <= 0)
        return false;

    *bypass_list = 0;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return false;

    char *list = utf8strdup(ie_config.lpszProxyBypass);
    if (list) {
        strncpy(bypass_list, list, max_bypass_list);
        bypass_list[max_bypass_list - 1] = 0;
        free(list);
        has_value = true;
    }

    free_winhttp_ie_proxy_config(&ie_config);
    return has_value;
}
