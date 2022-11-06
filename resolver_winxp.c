#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <windows.h>
#include <winhttp.h>

#include "log.h"
#include "resolver.h"
#include "resolver_i.h"
#include "resolver_winxp.h"
#include "util_win.h"

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

static void proxy_resolver_winxp_cleanup(proxy_resolver_winxp_s *proxy_resolver) {
    free(proxy_resolver->list);
    proxy_resolver->list = NULL;
}

static void proxy_resolver_winxp_reset(proxy_resolver_winxp_s *proxy_resolver) {
    proxy_resolver->pending = false;
    proxy_resolver->error = 0;

    proxy_resolver_winxp_cleanup(proxy_resolver);
}

bool proxy_resolver_winxp_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    WINHTTP_AUTOPROXY_OPTIONS options = {0};
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    WINHTTP_PROXY_INFO proxy_info = {0};
    wchar_t *url_wide = NULL;
    char *proxy = NULL;
    bool is_ok = false;

    proxy_resolver_winxp_reset(proxy_resolver);

    // Get current user proxy configuration
    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config)) {
        proxy_resolver->error = GetLastError();
        goto winxp_error;
    }

    // Set proxy options for calls to WinHttpGetProxyForUrl
    if (ie_config.lpszAutoConfigUrl) {
        // Use auto configuration script
        options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
        options.lpszAutoConfigUrl = ie_config.lpszAutoConfigUrl;
    } else if (ie_config.lpszProxy) {
        // Use explicit proxy list
        proxy_info.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        proxy_info.lpszProxy = ie_config.lpszProxy;
        goto winxp_done;
    } else if (!ie_config.fAutoDetect) {
        // Don't do automatic proxy detection
        goto winxp_done;
    } else {
        // Use WPAD to automatically retrieve proxy auto-configuration and evaluate it
        options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
        options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
    }

    // Convert url to wide char for WinHttpGetProxyForUrl
    url_wide = utf8_dup_to_wchar(url);
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
            int32_t error = GetLastError();

            // Failed to detect proxy auto configuration url so use DIRECT connection
            if (error == ERROR_WINHTTP_AUTODETECTION_FAILED) {
                LOG_DEBUG("Proxy resolution returned code (%d)\n", error);
                proxy_info.dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
                goto winxp_done;
            }

            proxy_resolver->error = error;
            goto winxp_error;
        }
    }

winxp_done:

    switch (proxy_info.dwAccessType) {
    case WINHTTP_ACCESS_TYPE_NO_PROXY:
        proxy_resolver->list = strdup("direct://");
        break;
    case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
        if (proxy_info.lpszProxy)
            proxy = wchar_dup_to_utf8(proxy_info.lpszProxy);
        if (!proxy)
            goto winxp_error;

        // Convert proxy list to uri list
        proxy_resolver->list = convert_proxy_list_to_uri_list(proxy);
        free(proxy);

        if (!proxy_resolver->list) {
            proxy_resolver->error = ERROR_OUTOFMEMORY;
            LOG_ERROR("Unable to allocate memory for proxy list (%" PRId32 ")", proxy_resolver->error);
            goto winxp_error;
        }
        break;
    }

winxp_error:

    proxy_resolver->pending = false;

    // Trigger user callback once done
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

    free(url_wide);

    // Free proxy info
    if (proxy_info.lpszProxy && proxy_info.lpszProxy != ie_config.lpszProxy)
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

const char *proxy_resolver_winxp_get_list(void *ctx) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (!proxy_resolver)
        return NULL;
    return proxy_resolver->list;
}

bool proxy_resolver_winxp_get_error(void *ctx, int32_t *error) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (!proxy_resolver || !error)
        return false;
    *error = proxy_resolver->error;
    return true;
}

bool proxy_resolver_winxp_is_pending(void *ctx) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (!proxy_resolver)
        return false;
    return proxy_resolver->pending;
}

bool proxy_resolver_winxp_cancel(void *ctx) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (!proxy_resolver)
        return false;
    if (proxy_resolver->resolver) {
        WinHttpCloseHandle(proxy_resolver->resolver);
        proxy_resolver->resolver = NULL;
    }
    return true;
}

bool proxy_resolver_winxp_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver->user_data = user_data;
    proxy_resolver->callback = callback;
    return true;
}

void *proxy_resolver_winxp_create(void) {
    proxy_resolver_winxp_s *proxy_resolver = (proxy_resolver_winxp_s *)calloc(1, sizeof(proxy_resolver_winxp_s));
    return proxy_resolver;
}

bool proxy_resolver_winxp_delete(void **ctx) {
    proxy_resolver_winxp_s *proxy_resolver;
    if (ctx == NULL)
        return false;
    proxy_resolver = (proxy_resolver_winxp_s *)*ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver_winxp_cancel(ctx);
    proxy_resolver_winxp_cleanup(proxy_resolver);
    free(proxy_resolver);
    return true;
}

bool proxy_resolver_winxp_is_async(void) {
    return false;
}

bool proxy_resolver_winxp_init(void) {
    g_proxy_resolver_winxp.session =
        WinHttpOpen(L"cproxyres", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (!g_proxy_resolver_winxp.session)
        return false;
    return true;
}

bool proxy_resolver_winxp_uninit(void) {
    if (g_proxy_resolver_winxp.session)
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
                                                        proxy_resolver_winxp_is_async,
                                                        proxy_resolver_winxp_init,
                                                        proxy_resolver_winxp_uninit};
    return &proxy_resolver_winxp_i;
}
