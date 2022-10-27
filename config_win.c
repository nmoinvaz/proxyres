#include <stdint.h>
#include <stdbool.h>

#include <windows.h>
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

char *proxy_config_win_get_auto_config_url(void) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    char *auto_config_url = NULL;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return NULL;

    if (ie_config.lpszAutoConfigUrl)
        auto_config_url = utf8strdup(ie_config.lpszAutoConfigUrl);

    free_winhttp_ie_proxy_config(&ie_config);
    return auto_config_url;
}

char *proxy_config_win_get_proxy(const char *protocol) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    char *list = NULL;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return NULL;

    if (ie_config.lpszProxy)
        list = utf8strdup(ie_config.lpszProxy);

    free_winhttp_ie_proxy_config(&ie_config);
    return list;
}

char *proxy_config_win_get_bypass_list(void) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    char *list = NULL;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return NULL;

    if (ie_config.lpszProxyBypass)
        list = utf8strdup(ie_config.lpszProxyBypass);

    free_winhttp_ie_proxy_config(&ie_config);
    return list;
}
