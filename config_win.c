#include <stdint.h>
#include <stdbool.h>

#include <windows.h>
#include <winhttp.h>

#include "config.h"
#include "config_i.h"
#include "config_win.h"

#include "util_win.h"

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
        auto_config_url = wchar_dup_to_utf8(ie_config.lpszAutoConfigUrl);

    free_winhttp_ie_proxy_config(&ie_config);
    return auto_config_url;
}

char *proxy_config_win_get_proxy(const char *protocol) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    char *list = NULL;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return NULL;

    if (ie_config.lpszProxy)
        list = wchar_dup_to_utf8(ie_config.lpszProxy);

    free_winhttp_ie_proxy_config(&ie_config);
    return list;
}

char *proxy_config_win_get_bypass_list(void) {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    char *list = NULL;

    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
        return NULL;

    if (ie_config.lpszProxyBypass)
        list = wchar_dup_to_utf8(ie_config.lpszProxyBypass);

    free_winhttp_ie_proxy_config(&ie_config);
    return list;
}

bool proxy_config_win_init(void) {
    return true;
}

bool proxy_config_win_uninit(void) {
    return true;
}

proxy_config_i_s *proxy_config_win_get_interface(void) {
    static proxy_config_i_s proxy_config_win_i = {proxy_config_win_get_auto_discover,
                                                  proxy_config_win_get_auto_config_url,
                                                  proxy_config_win_get_proxy,
                                                  proxy_config_win_get_bypass_list,
                                                  proxy_config_win_init,
                                                  proxy_config_win_uninit};
    return &proxy_config_win_i;
}
