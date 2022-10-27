#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <windows.h>
#include <winhttp.h>

#include "resolver.h"
#include "resolver_i.h"
#include "resolver_winxp.h"

#include "utils_win.h"

// WinHTTP proxy resolver function definitions
typedef DWORD(WINAPI *LPWINHTTPGETPROXYFORURLEX)(HINTERNET Resolver, PCWSTR Url,
                                                 WINHTTP_AUTOPROXY_OPTIONS *AutoProxyOptions, DWORD_PTR Context);
typedef DWORD(WINAPI *LPWINHTTPCREATEPROXYRESOLVER)(HINTERNET Session, HINTERNET *Resolver);
typedef DWORD(WINAPI *LPWINHTTPGETPROXYRESULT)(HINTERNET Resolver, WINHTTP_PROXY_RESULT *ProxyResult);
typedef VOID(WINAPI *LPWINHTTPFREEPROXYRESULT)(WINHTTP_PROXY_RESULT *ProxyResult);

typedef struct g_proxy_resolver_win8_s {
    // WinHTTP module handle
    HMODULE win_http;
    // WinHTTP session handle
    HINTERNET session;
    // WinHTTP proxy resolver functions
    LPWINHTTPGETPROXYFORURLEX winhttp_get_proxy_for_url_ex;
    LPWINHTTPCREATEPROXYRESOLVER winhttp_create_proxy_resolver;
    LPWINHTTPGETPROXYRESULT winhttp_get_proxy_result;
    LPWINHTTPFREEPROXYRESULT winhttp_free_proxy_result;
} g_proxy_resolver_win8_s;

struct g_proxy_resolver_win8_s g_proxy_resolver_win8;

typedef struct proxy_resolver_win8_s {
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
} proxy_resolver_win8_s;

static void proxy_resolver_win8_reset(proxy_resolver_win8_s *proxy_resolver) {
    proxy_resolver->pending = false;
    proxy_resolver->error = 0;
    if (proxy_resolver->list) {
        free(proxy_resolver->list);
        proxy_resolver->list = NULL;
    }
}

void CALLBACK proxy_resolver_win8_winhttp_status_callback(HINTERNET Internet, DWORD_PTR Context, DWORD InternetStatus,
                                                          LPVOID StatusInformation, DWORD StatusInformationLength) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)Context;
    WINHTTP_PROXY_RESULT proxy_result = {0};
    bool successful = false;

    if (InternetStatus == WINHTTP_CALLBACK_FLAG_REQUEST_ERROR) {
        WINHTTP_ASYNC_RESULT *async_result = (WINHTTP_ASYNC_RESULT *)StatusInformation;
        proxy_resolver->error = async_result->dwError;
        printf("Unable to retrieve proxy configuration (%d)\n", proxy_resolver->error);
        goto win8_complete;
    }

    proxy_resolver->error = g_proxy_resolver_win8.winhttp_get_proxy_result(proxy_resolver->resolver, &proxy_result);
    if (proxy_resolver->error != ERROR_SUCCESS) {
        printf("Unable to retrieve proxy result (%d)\n", proxy_resolver->error);
        goto win8_complete;
    }

    // Allocate string to construct the proxy list into
    int32_t max_list = proxy_result.cEntries * MAX_PROXY_URL;
    proxy_resolver->list = (char *)calloc(max_list, sizeof(char));
    if (proxy_resolver->list) {
        proxy_resolver->error = ERROR_OUTOFMEMORY;
        printf("Unable to allocate memory for proxy list\n");
        goto win8_complete;
    }

    size_t list_len = 0;

    // Construct proxy list string from WinHTTP proxy result
    for (uint32_t i = 0; i < proxy_result.cEntries; i++) {
        WINHTTP_PROXY_RESULT_ENTRY *entry = &proxy_result.pEntries[i];
        if (!entry)
            continue;

        // Prefix each URL with the proxy type
        if (entry->fProxy) {
            switch (entry->ProxyScheme) {
            case INTERNET_SCHEME_HTTP:
                strncat(proxy_resolver->list, "HTTP ", max_list - list_len);
                list_len += 5;
                break;
            case INTERNET_SCHEME_HTTPS:
                strncat(proxy_resolver->list, "HTTPS ", max_list - list_len);
                list_len += 6;
                break;
            case INTERNET_SCHEME_SOCKS:
                strncat(proxy_resolver->list, "SOCKS ", max_list - list_len);
                list_len += 6;
                break;
            }
            proxy_resolver->list[max_list - 1] = 0;

            // Convert wide char proxy url to UTF-8
            char *proxy_url = utf8strdup(entry->pwszProxy);
            if (proxy_url) {
                strncat(proxy_resolver->list, proxy_url, max_list - list_len);
                proxy_resolver->list[max_list - 1] = 0;
                list_len += strlen(proxy_url);
                free(proxy_url);
            }

            // Add proxy port to the end of the url
            if (entry->ProxyPort > 0) {
                snprintf(proxy_resolver->list + list_len, max_list - list_len, ":%d", entry->ProxyPort);
                proxy_resolver->list[max_list - 1] = 0;
                list_len = strlen(proxy_resolver->list);
            }
        } else {
            // No proxy
            strncat(proxy_resolver->list, "DIRECT", max_list - list_len);
            proxy_resolver->list[max_list - 1] = 0;
            list_len += 6;
        }

        // Separate each proxy url with a semicolon
        if (i != proxy_result.cEntries - 1) {
            strncat(proxy_resolver->list, ";", max_list - list_len);
            proxy_resolver->list[max_list - 1] = 0;
            list_len++;
        }
    }

win8_complete:

    // Free proxy result
    if (proxy_result.cEntries > 0)
        g_proxy_resolver_win8.winhttp_free_proxy_result(&proxy_result);

    proxy_resolver->pending = false;

    // Trigger user callback once done
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);
}

bool proxy_resolver_win8_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)ctx;
    WINHTTP_AUTOPROXY_OPTIONS options = {0};
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
    WINHTTP_PROXY_INFO proxy_info = {0};
    wchar_t *url_wide = NULL;
    char *proxy = NULL;
    bool is_ok = false;
    int32_t error = 0;

    if (!proxy_resolver || !url)
        return false;

    proxy_resolver_win8_reset(proxy_resolver);

    // Get current user proxy configuration
    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_config)) {
        proxy_resolver->error = GetLastError();
        return false;
    }

    // Set proxy options for calls to WinHttpGetProxyForUrl
    if (ie_config.lpszProxy != NULL) {
        // Use explicit proxy list
        proxy_info.lpszProxy = ie_config.lpszProxy;
        goto win8_done;
    } else if (ie_config.lpszAutoConfigUrl != NULL) {
        // Use auto configuration script
        options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
        options.lpszAutoConfigUrl = ie_config.lpszAutoConfigUrl;
    } else if (!ie_config.fAutoDetect) {
        // Don't do automatic proxy detection
        goto win8_done;
    } else {
        // Use WPAD to automatically retrieve proxy auto-configuration and evaluate it
        options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
        options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
    }

    g_proxy_resolver_win8.winhttp_create_proxy_resolver(g_proxy_resolver_win8.session, &proxy_resolver->resolver);
    if (!proxy_resolver->resolver) {
        proxy_resolver->error = GetLastError();
        printf("Unable to create proxy resolver (%d)", proxy_resolver->error);
        goto win8_error;
    }

    if (WinHttpSetStatusCallback(proxy_resolver->resolver,
                                 (WINHTTP_STATUS_CALLBACK)proxy_resolver_win8_winhttp_status_callback,
                                 WINHTTP_CALLBACK_FLAG_REQUEST_ERROR | WINHTTP_CALLBACK_FLAG_GETPROXYFORURL_COMPLETE,
                                 (DWORD_PTR)NULL) == WINHTTP_INVALID_STATUS_CALLBACK) {
        proxy_resolver->error = GetLastError();
        printf("Unable to install status callback (%d)", proxy_resolver->error);
        goto win8_error;
    }

    // Convert url to wide char for WinHttpGetProxyForUrlEx
    url_wide = wstrdup(url);
    if (!url_wide)
        goto win8_error;

    // For performance reasons try fAutoLogonIfChallenged = false then try fAutoLogonIfChallenged = true
    // https://docs.microsoft.com/en-us/windows/win32/api/winhttp/ns-winhttp-winhttp_autoproxy_options

    error = g_proxy_resolver_win8.winhttp_get_proxy_for_url_ex(proxy_resolver->resolver, url_wide, &options,
                                                               (DWORD_PTR)proxy_resolver);
    if (error == ERROR_WINHTTP_LOGIN_FAILURE) {
        options.fAutoLogonIfChallenged = true;
        error = g_proxy_resolver_win8.winhttp_get_proxy_for_url_ex(proxy_resolver->resolver, url_wide, &options,
                                                                   (DWORD_PTR)proxy_resolver);
    }

    if (error != ERROR_IO_PENDING) {
        proxy_resolver->error = error;
        printf("Unable to get proxy for url %s (%d)", url, proxy_resolver->error);
        goto win8_error;
    }

    proxy_resolver->pending = true;
    goto win8_cleanup;

win8_done:
    // If proxy is null then no proxy is used
    if (!proxy_info.lpszProxy)
        goto win8_ok;

    // Copy proxy list to proxy resolver
    proxy = utf8strdup(proxy_info.lpszProxy);
    size_t max_list = strlen(proxy) + 1;
    proxy_resolver->list = (char *)calloc(max_list, sizeof(char));

    if (!proxy_resolver->list) {
        proxy_resolver->error = ERROR_OUTOFMEMORY;
        printf("Unable to allocate memory for proxy list (%d)", proxy_resolver->error);
        goto win8_error;
    }

    strncat(proxy_resolver->list, proxy, max_list);
    proxy_resolver->list[max_list - 1] = 0;

win8_error:
win8_ok:

    proxy_resolver->pending = false;

    // Trigger user callback once done
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

win8_cleanup:

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

bool proxy_resolver_win8_get_list(void *ctx, char **list) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)ctx;
    if (!proxy_resolver || !list)
        return false;
    *list = proxy_resolver->list;
    return (*list != NULL);
}

bool proxy_resolver_win8_get_error(void *ctx, int32_t *error) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)ctx;
    if (!proxy_resolver || !error)
        return false;
    *error = proxy_resolver->error;
    return true;
}

bool proxy_resolver_win8_is_pending(void *ctx) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)ctx;
    if (!proxy_resolver)
        return false;
    return proxy_resolver->pending;
}

bool proxy_resolver_win8_cancel(void *ctx) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)ctx;
    if (!proxy_resolver)
        return false;
    if (proxy_resolver->resolver) {
        WinHttpCloseHandle(proxy_resolver->resolver);
        proxy_resolver->resolver = NULL;
    }
    return true;
}

bool proxy_resolver_win8_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver->user_data = user_data;
    proxy_resolver->callback = callback;
    return true;
}

bool proxy_resolver_win8_create(void **ctx) {
    proxy_resolver_win8_s *proxy_resolver = (proxy_resolver_win8_s *)calloc(1, sizeof(proxy_resolver_win8_s));
    if (!proxy_resolver)
        return false;
    *ctx = proxy_resolver;
    return true;
}

bool proxy_resolver_win8_delete(void **ctx) {
    proxy_resolver_win8_s *proxy_resolver;
    if (ctx == NULL)
        return false;
    proxy_resolver = (proxy_resolver_win8_s *)*ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver_win8_cancel(ctx);
    free(proxy_resolver);
    return true;
}

bool proxy_resolver_win8_init(void) {
    // Dynamically load WinHTTP and CreateProxyResolver which is only avaialble on Windows 8 or higher
    g_proxy_resolver_win8.win_http = LoadLibraryExA("winhttp.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!g_proxy_resolver_win8.win_http)
        goto win8_init_error;

    g_proxy_resolver_win8.winhttp_create_proxy_resolver =
        (LPWINHTTPCREATEPROXYRESOLVER)GetProcAddress(g_proxy_resolver_win8.win_http, "WinHttpCreateProxyResolver");
    if (!g_proxy_resolver_win8.winhttp_create_proxy_resolver)
        goto win8_init_error;
    g_proxy_resolver_win8.winhttp_get_proxy_for_url_ex =
        (LPWINHTTPGETPROXYFORURLEX)GetProcAddress(g_proxy_resolver_win8.win_http, "WinHttpGetProxyForUrlEx");
    if (!g_proxy_resolver_win8.winhttp_get_proxy_for_url_ex)
        goto win8_init_error;
    g_proxy_resolver_win8.winhttp_get_proxy_result =
        (LPWINHTTPGETPROXYRESULT)GetProcAddress(g_proxy_resolver_win8.win_http, "WinHttpGetProxyResult");
    if (!g_proxy_resolver_win8.winhttp_get_proxy_result)
        goto win8_init_error;
    g_proxy_resolver_win8.winhttp_free_proxy_result =
        (LPWINHTTPFREEPROXYRESULT)GetProcAddress(g_proxy_resolver_win8.win_http, "WinHttpFreeProxyResult");
    if (!g_proxy_resolver_win8.winhttp_free_proxy_result)
        goto win8_init_error;

    if (g_proxy_resolver_win8.winhttp_create_proxy_resolver) {
        g_proxy_resolver_win8.session = WinHttpOpen(L"cproxyres", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    }

    if (!g_proxy_resolver_win8.session) {
    win8_init_error:
        FreeLibrary(g_proxy_resolver_win8.win_http);
        g_proxy_resolver_win8.win_http = NULL;
        return false;
    }

    return true;
}

bool proxy_resolver_win8_uninit(void) {
    if (g_proxy_resolver_win8.session)
        WinHttpCloseHandle(g_proxy_resolver_win8.session);
    if (g_proxy_resolver_win8.win_http)
        FreeLibrary(g_proxy_resolver_win8.win_http);

    memset(&g_proxy_resolver_win8, 0, sizeof(g_proxy_resolver_win8));
    return true;
}

proxy_resolver_i_s *proxy_resolver_win8_get_interface(void) {
    static proxy_resolver_i_s proxy_resolver_win8_i = {proxy_resolver_win8_get_proxies_for_url,
                                                       proxy_resolver_win8_get_list,
                                                       proxy_resolver_win8_get_error,
                                                       proxy_resolver_win8_is_pending,
                                                       proxy_resolver_win8_cancel,
                                                       proxy_resolver_win8_set_resolved_callback,
                                                       proxy_resolver_win8_create,
                                                       proxy_resolver_win8_delete,
                                                       proxy_resolver_win8_init,
                                                       proxy_resolver_win8_uninit};
    return &proxy_resolver_win8_i;
}
