#include <stdint.h>
#include <wchar.h>

#include <windows.h>

#include "util.h"

// Create a wide char string from a UTF-8 string
wchar_t *utf8_dup_to_wchar(const char *src) {
    int32_t len = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    wchar_t *dup = (wchar_t *)calloc(len, sizeof(wchar_t));
    if (dup == NULL)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, src, -1, dup, len);
    return dup;
}

// Create a UTF-8 string from a wide char string
char *wchar_dup_to_utf8(const wchar_t *src) {
    int32_t len = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
    char *dup = (char *)calloc(len, sizeof(char));
    if (dup == NULL)
        return NULL;
    WideCharToMultiByte(CP_UTF8, 0, src, -1, dup, len, NULL, NULL);
    return dup;
}

// Get proxy by scheme from a WinHTTP proxy list.
// The proxy list contains one or more strings separated by a semicolons or whitespace:
//    ([<scheme>=][<scheme>"://"]<server>[":"<port>])
char *get_winhttp_proxy_by_scheme(const char *url_or_scheme, const char *proxy_list) {
    size_t proxy_list_len = strlen(proxy_list);
    const char *proxy_list_end = proxy_list + proxy_list_len;

    // Get scheme from url
    char *url_scheme = get_url_scheme(url_or_scheme, url_or_scheme);
    size_t url_scheme_len = strlen(url_scheme);

    const char *config_start = proxy_list;
    const char *config_end = NULL;

    // Enumerate each proxy in the proxy list.
    do {
        // Proxies can be separated by a semi-colon or a whitespace
        config_end = str_find_first_char(config_start, "; \t\r\n");
        if (!config_end)
            config_end = config_start + strlen(config_start);

        // Find scheme boundary
        const char *scheme_end = config_start;
        while (scheme_end < config_end && *scheme_end) {
            if ((*scheme_end == '=') || (*scheme_end == ':' && scheme_end[1] == '/' && scheme_end[2] == '/')) {
                break;
            }
            scheme_end++;
        }

        const char *scheme = NULL;
        size_t scheme_len = 0;
        const char *host_start = NULL;
        size_t host_len = 0;

        if (scheme_end == config_end) {
            // No scheme, assume http
            scheme = "http";
            scheme_len = 4;
            // Calculate start of host
            host_start = config_start;
        } else {
            // Copy the proxy scheme
            scheme = config_start;
            scheme_len = (size_t)(scheme_end - config_start);
            // Calculate start of host
            host_start = scheme_end;
            while (*host_start == '=' || *host_start == ':' || *host_start == '/')
                host_start++;
        }

        // Check to see if the scheme in the proxy config matches the url scheme
        if (url_scheme_len == scheme_len && _strnicmp(url_scheme, scheme, scheme_len) == 0) {
            if (!config_end)
                config_end = host_start + strlen(host_start);

            // Return matching proxy config in proxy list
            size_t uri_list_len = (size_t)(config_end - host_start);
            char *proxy = (char *)calloc(uri_list_len + 1, sizeof(char));
            if (proxy)
                strncat(proxy, host_start, uri_list_len);
            return proxy;
        }

        // Continue to next proxy
        config_start = config_end + 1;
    } while (config_end < proxy_list_end);

    return strdup(proxy_list);
}

// Convert WinHTTP proxy list to uri list.
// The proxy list contains one or more strings separated by a semicolons or whitespace:
//    ([<scheme>=][<scheme>"://"]<server>[":"<port>])
char *convert_winhttp_proxy_list_to_uri_list(const char *proxy_list) {
    if (!proxy_list)
        return NULL;

    size_t proxy_list_len = strlen(proxy_list);
    const char *proxy_list_end = proxy_list + proxy_list_len;

    size_t uri_list_len = 0;
    size_t max_uri_list = proxy_list_len + 128;  // Extra space for scheme separators
    char *uri_list = (char *)calloc(max_uri_list, sizeof(char));
    if (!uri_list)
        return NULL;

    const char *config_start = proxy_list;
    const char *config_end = NULL;
    char *host_start = NULL;

    // Enumerate each proxy in the proxy list.
    do {
        // Proxies can be separated by a semi-colon or a whitespace
        config_end = str_find_first_char(config_start, "; \t\r\n");
        if (!config_end)
            config_end = config_start + strlen(config_start);

        // Find scheme boundary
        const char *scheme_end = config_start;
        while (scheme_end < config_end && *scheme_end) {
            if ((*scheme_end == '=') || (*scheme_end == ':' && scheme_end[1] == '/' && scheme_end[2] == '/')) {
                break;
            }
            scheme_end++;
        }

        const char *scheme = NULL;
        size_t scheme_len = 0;
        const char *host_start = NULL;
        size_t host_len = 0;

        if (scheme_end == config_end) {
            // No scheme, assume http
            scheme = "http";
            scheme_len = 4;
            // Calculate start of host
            host_start = config_start;
        } else {
            // Copy the proxy scheme
            scheme = config_start;
            scheme_len = (int32_t)(scheme_end - config_start);
            // Calculate start of host
            host_start = scheme_end;
            while (*host_start == '=' || *host_start == ':' || *host_start == '/')
                host_start++;
        }

        // Copy scheme
        if (scheme_len > max_uri_list - 1)
            scheme_len = max_uri_list - 1;
        strncat(uri_list, scheme, scheme_len);
        uri_list_len += scheme_len;

        strncat(uri_list, "://", max_uri_list - uri_list_len - 1);
        uri_list_len += 3;

        // Copy proxy address
        host_len = (size_t)(config_end - host_start);
        if (host_len > max_uri_list - 1)
            host_len = max_uri_list - 1;
        strncat(uri_list, host_start, host_len);
        uri_list_len += host_len;

        // Separate each proxy with comma
        if (config_end != proxy_list_end) {
            strncat(uri_list, ",", max_uri_list - uri_list_len - 1);
            uri_list_len += 1;
        }

        // Continue to next proxy
        config_start = config_end + 1;
    } while (config_end < proxy_list_end);

    return uri_list;
}
