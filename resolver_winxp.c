#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <winhttp.h>

#include "resolver.h"
#include "resolver_i.h"
#include "resolver_winxp.h"

#include "utils_win.h"

typedef struct g_proxy_resolver_winxp_s {
    // WinHTTP module handle
    HMODULE win_http;
    // WinHTTP session handle
    HINTERNET session;
} g_proxy_resolver_win2k_s;

struct g_proxy_resolver_winxp_s g_proxy_resolver_winxp;

typedef struct proxy_resolver_winxp_s {
    // WinHTTP proxy resolver handle
    HINTERNET resolver;
    // Last system error
    int32_t error;
    // Resolved user callback
    void *user_data;
    proxy_resolver_resolved_cb callback;
    // Resolution pending
    bool pending;
    // Proxy list
    char *list;
} proxy_resolver_winxp_s;

static void proxy_resolver_winxp_reset(proxy_resolver_winxp_s *proxy_resolver) {
    proxy_resolver->pending = false;
    proxy_resolver->error = 0;
    if (proxy_resolver->list) {
        free(proxy_resolver->list);
        proxy_resolver->list = NULL;
    }
}

static bool proxy_resolver_winxp_get_proxies_for_url_thread(proxy_resolver_winxp_s *proxy_resolver, const char *url) {
    WINHTTP_AUTOPROXY_OPTIONS options = {0};
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    WINHTTP_PROXY_INFO proxy_info = {0};
    wchar_t *url_wide = NULL;
    char *proxy_wide = NULL;
    bool is_ok = false;

    proxy_resolver_winxp_reset(proxy_resolver);

    // Get current user proxy configuration
    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config)) {
        proxy_resolver->error = GetLastError();
        goto winxp_error;
    }

    // Set proxy options for calls to WinHttpGetProxyForUrl
    if (ie_config.lpszProxy != NULL) {
        // Use explicit proxy list
        proxy_info.lpszProxy = ie_config.lpszProxy;
        goto winxp_done;
    } else if (ie_config.lpszAutoConfigUrl != NULL) {
        // Use auto configuration script
        options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
        options.lpszAutoConfigUrl = ie_config.lpszAutoConfigUrl;
    } else if (!ie_config.fAutoDetect) {
        // Don't do automatic proxy detection
        goto winxp_done;
    } else {
        // Use WPAD to automatically retrieve proxy auto-configuration and evaluate it
        options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
        options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
    }

    // Convert url to wide char for WinHttpGetProxyForUrl
    url_wide = wstrdup(url);
    if (!url_wide)
        goto winxp_error;

    // For performance reasons try fAutoLogonIfChallenged = false then try fAutoLogonIfChallenged = true
    // https://docs.microsoft.com/en-us/windows/win32/api/winhttp/ns-winhttp-winhttp_autoproxy_options

    is_ok = WinHttpGetProxyForUrl(g_proxy_resolver_winxp.session, url_wide, &options, &proxy_info);
    if (!is_ok) {
        if (GetLastError() == ERROR_WINHTTP_LOGIN_FAILURE) {
            options.fAutoLogonIfChallenged = true;
            is_ok = WinHttpGetProxyForUrl(g_proxy_resolver_winxp.session, url_wide, &options, &proxy_info);
        }
        if (!is_ok) {
            proxy_resolver->error = GetLastError();
            goto winxp_error;
        }
    }

winxp_done:

    // If proxy is null then no proxy is used
    if (proxy_info.lpszProxy == NULL)
        goto winxp_ok;

    // Copy proxy list to proxy resolver
    proxy_wide = utf8strdup(proxy_info.lpszProxy);
    size_t max_list = strlen(proxy_wide) + 1;
    proxy_resolver->list = (char *)calloc(max_list, sizeof(char));

    if (proxy_resolver->list == NULL) {
        proxy_resolver->error = ERROR_OUTOFMEMORY;
        printf("Unable to allocate memory for proxy list (%d)", proxy_resolver->error);
        goto winxp_error;
    }

    strncat(proxy_resolver->list, proxy_wide, max_list);
    proxy_resolver->list[max_list - 1] = 0;

winxp_error:
winxp_ok:

    proxy_resolver->pending = false;

    // Trigger user callback once done
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

    free(proxy_wide);
    free(url_wide);

    // Free proxy info
    if (proxy_info.lpszProxy)
        GlobalFree(proxy_info.lpszProxy);
    if (proxy_info.lpszProxyBypass)
        GlobalFree(proxy_info.lpszProxyBypass);

    // Free IE proxy configuration
    if (ie_config.lpszProxy)
        GlobalFree(ie_config.lpszProxy);
    if (ie_config.lpszProxyBypass)
        GlobalFree(ie_config.lpszProxyBypass);
    if (ie_config.lpszAutoConfigUrl)
        GlobalFree(ie_config.lpszAutoConfigUrl);

    return proxy_resolver->error == 0;
}

bool proxy_resolver_winxp_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (proxy_resolver == NULL || url == NULL)
        return false;

    // TODO: Add threading
    return proxy_resolver_winxp_get_proxies_for_url_thread(proxy_resolver, url);
}

bool proxy_resolver_winxp_get_list(void *ctx, char **list) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (proxy_resolver == NULL || list == NULL)
        return false;
    *list = proxy_resolver->list;
    return (*list != NULL);
}

bool proxy_resolver_winxp_get_error(void *ctx, int32_t *error) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (proxy_resolver == NULL || error == NULL)
        return false;
    *error = proxy_resolver->error;
    return true;
}

bool proxy_resolver_winxp_is_pending(void *ctx) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (proxy_resolver == NULL)
        return false;
    return proxy_resolver->pending;
}

bool proxy_resolver_winxp_cancel(void *ctx) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (proxy_resolver == NULL)
        return false;
    if (proxy_resolver->resolver) {
        WinHttpCloseHandle(proxy_resolver->resolver);
        proxy_resolver->resolver = NULL;
    }
    return true;
}

bool proxy_resolver_winxp_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (proxy_resolver == NULL)
        return false;
    proxy_resolver->user_data = user_data;
    proxy_resolver->callback = callback;
    return true;
}

bool proxy_resolver_winxp_create(void **ctx) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)calloc(1, sizeof(proxy_resolver_winxp_s));
    if (proxy_resolver == NULL)
        return false;
    *ctx = proxy_resolver;
    return true;
}

bool proxy_resolver_winxp_delete(void **ctx) {
    proxy_resolver_winxp_s *proxy_resolver;
    if (ctx == NULL)
        return false;
    proxy_resolver = (proxy_resolver_winxp_s *)*ctx;
    if (proxy_resolver == NULL)
        return false;
    proxy_resolver_winxp_cancel(ctx);
    free(proxy_resolver);
    return true;
}

bool proxy_resolver_winxp_init(void) {
    memset(&g_proxy_resolver_winxp, 0, sizeof(g_proxy_resolver_winxp));

    g_proxy_resolver_winxp.session =
        WinHttpOpen(L"cproxyres", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (g_proxy_resolver_winxp.session == NULL)
        return false;
    return true;
}

bool proxy_resolver_winxp_uninit(void) {
    if (g_proxy_resolver_winxp.session != NULL)
        WinHttpCloseHandle(g_proxy_resolver_winxp.session);

    memset(&g_proxy_resolver_winxp, 0, sizeof(g_proxy_resolver_winxp));
    return true;
}

proxy_resolver_i_s *proxy_resolver_winxp_get_interface(void) {
    static proxy_resolver_i_s proxy_resolver_winxp_i = {proxy_resolver_winxp_get_proxies_for_url,
                                                        proxy_resolver_winxp_get_list,
                                                        proxy_resolver_winxp_get_error,
                                                        proxy_resolver_winxp_is_pending,
                                                        proxy_resolver_winxp_cancel,
                                                        proxy_resolver_winxp_set_resolved_callback,
                                                        proxy_resolver_winxp_create,
                                                        proxy_resolver_winxp_delete,
                                                        proxy_resolver_winxp_uninit};
    return &proxy_resolver_winxp_i;
}