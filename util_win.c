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

// Get proxy by protocol from a proxy list that is in the following format:
//  ([<scheme>=][<scheme>"://"]<server>[":"<port>])
char *get_proxy_by_protocol(const char *protocol, const char *proxy_list) {
    char *host = NULL;
    const char *protocol_start = protocol;
    size_t protocol_len = 0;

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
            int32_t uri_list_len = (int32_t)(config_end - address_start);
            char *proxy = (char *)calloc(uri_list_len + 1, sizeof(char));
            if (proxy)
                strncat(proxy, address_start, uri_list_len);
            return proxy;
        }
        if (!config_end)
            break;

        // Continue to next proxy config
        config_start = config_end + 1;
    }

    return strdup(proxy_list);
}

// Convert WinHTTP proxy list schema to uri list
char *convert_proxy_list_to_uri_list(const char *proxy_list) {
    if (!proxy_list)
        return NULL;

    size_t proxy_list_len = strlen(proxy_list);
    const char *proxy_list_end = proxy_list + proxy_list_len;

    size_t uri_list_len = 0;
    size_t max_uri_list = proxy_list_len + 128;  // Extra space for schema separators
    char *uri_list = (char *)calloc(max_uri_list, sizeof(char));
    if (!uri_list)
        return NULL;

    const char *config_start = proxy_list;
    const char *config_end = NULL;
    char *address_start = NULL;

    // Enumerate each proxy in the proxy list.
    do {
        // Proxies can be separated by a semi-colon or a whitespace
        config_end = str_find_first_char(config_start, "; \t\r\n");
        if (!config_end)
            config_end = config_start + strlen(config_start);

        // Find start of proxy address
        const char *schema_end = str_find_len_first_char(config_start, (int32_t)(config_end - config_start), ":=");

        const char *protocol = NULL;
        size_t protocol_len = 0;
        const char *address_start = NULL;
        size_t address_len = 0;

        if (!schema_end) {
            // No schema, assume http
            protocol = "http";
            protocol_len = 4;
            // Calculate start of proxy address
            address_start = config_start;
        } else {
            // Copy the proxy schema
            protocol = config_start;
            protocol_len = (int32_t)(schema_end - config_start);
            // Calculate start of proxy address
            address_start = schema_end;
            while (*address_start == '=' || *address_start == ':' || *address_start == '/')
                address_start++;
        }

        // Copy protocol
        if (protocol_len > max_uri_list - 1)
            protocol_len = max_uri_list - 1;
        strncat(uri_list, protocol, protocol_len);
        uri_list_len += protocol_len;

        strncat(uri_list, "://", max_uri_list - uri_list_len - 1);
        uri_list_len += 3;

        // Copy proxy address
        address_len = (int32_t)(config_end - address_start);
        if (address_len > max_uri_list - 1)
            address_len = max_uri_list - 1;
        strncat(uri_list, address_start, address_len);
        uri_list_len += address_len;

        // Separate each proxy with comma
        if (config_end != proxy_list_end) {
            strncat(uri_list, ",", max_uri_list - uri_list_len - 1);
            uri_list_len += 1;
        }

        config_start = config_end + 1;
    } while (config_end < proxy_list_end);

    return uri_list;
}
