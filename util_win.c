#include <stdint.h>
#include <wchar.h>

#include <windows.h>

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

// Get proxy by protocol from proxy list in the format of rotocol1=url1;protocol2=url2.
char *get_proxy_by_protocol(const char *protocol, const char *proxy_list) {
    char *host = NULL;
    const char *protocol_start = protocol;
    int32_t protocol_len = 0;

    // Get protocol start and length if incase input is a url
    host = strstr(protocol, "://");
    if (host) {
        protocol_len = (int32_t)(host - protocol);
    } else if (strcmpi(protocol, "http") && strcmpi(protocol, "https") && strcmpi(protocol, "socks")) {
        protocol_start = "http";
        protocol_len = 4;
    } else {
        protocol_len = strlen(protocol);
    }

    const char *config_start = proxy_list;
    char *config_end = NULL;
    char *address_start = NULL;

    // Enumerate each proxy in the proxy list
    while ((address_start = strchr(config_start, '=')) != NULL) {
        address_start++;
        config_end = strchr(address_start, ';');

        // Check to see if the key in the proxy config matches the protocol
        if (_strnicmp(config_start, protocol_start, protocol_len) == 0 && config_start[protocol_len] == '=') {
            if (!config_end)
                config_end = address_start + strlen(address_start);

            // Return matching proxy config in proxy list
            int32_t proxy_len = (int32_t)(config_end - address_start);
            char *proxy = (char *)calloc(proxy_len + 1, sizeof(char));
            if (proxy)
                strncpy(proxy, address_start, proxy_len);
            return proxy;
        }
        if (!config_end)
            break;

        // Continue to next proxy config
        config_start = config_end + 1;
    }

    return strdup(proxy_list);
}