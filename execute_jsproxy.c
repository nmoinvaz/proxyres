#include <stdint.h>
#include <stdbool.h>

#include <windows.h>
#include <wininet.h>
#include <winineti.h>

#include "execute.h"
#include "execute_jsproxy.h"

#include "util.h"

typedef struct g_proxy_execute_jsproxy_s {
    // JSProxy module
    HMODULE module;
    // JSProxy functions
    pfnInternetInitializeAutoProxyDll InternetInitializeAutoProxyDll;
    pfnInternetDeInitializeAutoProxyDll InternetDeInitializeAutoProxyDll;
    pfnInternetGetProxyInfo InternetGetProxyInfo;
} g_proxy_execute_jsproxy_s;

g_proxy_execute_jsproxy_s g_proxy_execute_jsproxy;

typedef struct proxy_execute_jsproxy_s {
    // Last error
    int32_t error;
    // Proxy list
    char *list;
}
proxy_execute_jsproxy_s;

DWORD CALLBACK proxy_execute_jsproxy_dns_resolve(LPSTR host, LPSTR ip, LPDWORD ip_size) {
    char *address = dns_resolve(host, NULL);
    if (!address)
        return ERROR_INTERNET_NAME_NOT_RESOLVED;

    size_t address_len = strlen(address);
    if (*ip_size < address_len) {
        free(address);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *ip_size = (DWORD)address_len;
    strncpy(ip, address, *ip_size);
    ip[address_len] = 0;
    free(address);
    return ERROR_SUCCESS;
}

BOOL CALLBACK proxy_execute_jsproxy_is_resolvable(LPSTR host) {
    char ip[256] = {0};
    DWORD ip_size = sizeof(ip);
    if (proxy_execute_jsproxy_dns_resolve(host, ip, &ip_size) != ERROR_SUCCESS)
        return false;
    return true;
}

DWORD CALLBACK proxy_execute_jsproxy_my_ip_address(LPSTR ip, LPDWORD ip_size) {
    return proxy_execute_jsproxy_dns_resolve(NULL, ip, ip_size);
}

BOOL CALLBACK proxy_execute_jsproxy_is_in_net(LPSTR ip, LPSTR target, LPSTR mask) {
    uint32_t ip_int = inet_addr(ip);
    if (ip_int == INADDR_NONE)
        return false;
    uint32_t target_int = inet_addr(target);
    if (target_int == INADDR_NONE)
        return false;
    uint32_t mask_int = inet_addr(mask);
    if ((ip_int & mask_int) != target_int)
        return false;

    return true;
}

bool proxy_execute_jsproxy_get_proxies_for_url(void *ctx, const char *script, const char *url) {
    proxy_execute_jsproxy_s *proxy_execute_jsproxy = (proxy_execute_jsproxy_s *)ctx;
    AutoProxyHelperVtbl ap_table = {&proxy_execute_jsproxy_is_resolvable,
                                    &proxy_execute_jsproxy_my_ip_address,
                                    &proxy_execute_jsproxy_dns_resolve,
                                    &proxy_execute_jsproxy_is_in_net,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL};
    AutoProxyHelperFunctions ap_helpers = {&ap_table};
    AUTO_PROXY_SCRIPT_BUFFER ap_script = {sizeof(AUTO_PROXY_SCRIPT_BUFFER), (char *)script, (DWORD)strlen(script)};
    DWORD proxy_len = 0;
    char *proxy = NULL;
    bool success = false;

    if (g_proxy_execute_jsproxy.InternetInitializeAutoProxyDll(0, 0, 0, &ap_helpers, &ap_script) == false)
        return false;

    // Don't include port in FindProxyForURL function
    char *host = parse_url_host(url);
    if (!host)
        return false;

    success =
        g_proxy_execute_jsproxy.InternetGetProxyInfo(url, (DWORD)strlen(url) + 1, host, (DWORD)strlen(host) + 1, &proxy, &proxy_len);
    free(host);

    proxy_execute_jsproxy->error = GetLastError();
    proxy_execute_jsproxy->list = success ? strdup(proxy) : NULL;

    if (proxy)
        GlobalFree(proxy);

    g_proxy_execute_jsproxy.InternetDeInitializeAutoProxyDll(NULL, 0);
    return success;
}

char *proxy_execute_jsproxy_get_list(void *ctx) {
    proxy_execute_jsproxy_s *proxy_execute = (proxy_execute_jsproxy_s *)ctx;
    return proxy_execute->list;
}

bool proxy_execute_jsproxy_get_error(void *ctx, int32_t *error) {
    proxy_execute_jsproxy_s *proxy_execute = (proxy_execute_jsproxy_s *)ctx;
    *error = proxy_execute->error;
    return true;
}

bool proxy_execute_jsproxy_create(void **ctx) {
    proxy_execute_jsproxy_s *proxy_execute =
        (proxy_execute_jsproxy_s *)calloc(1, sizeof(proxy_execute_jsproxy_s));
    if (!proxy_execute)
        return false;
    *ctx = proxy_execute;
    return true;
}

bool proxy_execute_jsproxy_delete(void **ctx) {
    proxy_execute_jsproxy_s *proxy_execute;
    if (!ctx)
        return false;
    proxy_execute = (proxy_execute_jsproxy_s *)*ctx;
    if (!proxy_execute)
        return false;
    free(proxy_execute->list);
    free(proxy_execute);
    *ctx = NULL;
    return true;
}

bool proxy_execute_jsproxy_init(void) {
    g_proxy_execute_jsproxy.module = LoadLibrary("jsproxy.dll");
    if (!g_proxy_execute_jsproxy.module)
        return false;

    g_proxy_execute_jsproxy.InternetInitializeAutoProxyDll = (pfnInternetInitializeAutoProxyDll)GetProcAddress(
        g_proxy_execute_jsproxy.module, "InternetInitializeAutoProxyDll");
    if (!g_proxy_execute_jsproxy.InternetInitializeAutoProxyDll)
        goto jsproxy_init_err;
    g_proxy_execute_jsproxy.InternetDeInitializeAutoProxyDll = (pfnInternetDeInitializeAutoProxyDll)GetProcAddress(
        g_proxy_execute_jsproxy.module, "InternetDeInitializeAutoProxyDll");
    if (!g_proxy_execute_jsproxy.InternetDeInitializeAutoProxyDll)
        goto jsproxy_init_err;
    g_proxy_execute_jsproxy.InternetGetProxyInfo =
        (pfnInternetGetProxyInfo)GetProcAddress(g_proxy_execute_jsproxy.module, "InternetGetProxyInfo");
    if (!g_proxy_execute_jsproxy.InternetGetProxyInfo)
        goto jsproxy_init_err;
    
    return true;
    
jsproxy_init_err:
    proxy_execute_jsproxy_uninit();
    return false;
}

bool proxy_execute_jsproxy_uninit(void) {
    if (g_proxy_execute_jsproxy.module)
        FreeLibrary(g_proxy_execute_jsproxy.module);

    memset(&g_proxy_execute_jsproxy, 0, sizeof(g_proxy_execute_jsproxy));
    return true;
}