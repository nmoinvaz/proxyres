#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

#include "util.h"

// Replace one character in the string with another
int32_t str_change_chr(char *str, char from, char to) {
    int32_t count = 0;
    while (*str) {
        if (*str == from) {
            *str = to;
            count++;
        }
        str++;
    }
    return count;
}

// Trim a character from the end of the string
int32_t str_trim_end(char *str, char c) {
    int32_t count = 0;
    char *end = str + strlen(str) - 1;
    while (end >= str && *end == c) {
        *end = 0;
        end--;
        count++;
    }
    return count;
}

// Find first character in string
const char *str_find_first_char(const char *str, const char *chars) {
    while (*str) {
        const char *c = chars;
        while (*c) {
            if (*str == *c)
                return str;
            c++;
        }
        str++;
    }
    return NULL;
}

// Find character in string up to max length
const char *str_find_len_char(const char *str, size_t str_len, char c) {
    while (str_len && *str) {
        if (*str == c)
            return str;
        str++;
        str_len--;
    }
    return NULL;
}

// Find string in string up to max length
const char *str_find_len_str(const char *str, size_t str_len, const char *find) {
    size_t find_len = strlen(find);
    while (str_len >= find_len && *str) {
        if (strncmp(str, find, find_len) == 0)
            return str;
        str++;
        str_len--;
    }
    return NULL;
}

// Find string case-insensitve in string up to max length
const char *str_find_len_case_str(const char *str, size_t str_len, const char *find) {
    size_t find_len = strlen(find);
    while (str_len >= find_len && *str) {
        if (strncasecmp(str, find, find_len) == 0)
            return str;
        str++;
        str_len--;
    }
    return NULL;
}

// Find host for a given url
char *get_url_host(const char *url) {
    // Find the start of the host after the scheme
    const char *host_start = strstr(url, "://");
    if (host_start)
        host_start += 3;
    else
        host_start = url;

    // Skip username and password
    const char *at_start = strchr(host_start, '@');
    if (at_start)
        host_start = at_start + 1;

    // Find the end of the host
    const char *host_end = strstr(host_start, "/");
    if (!host_end)
        host_end = (char *)host_start + strlen(host_start);

    // Allocate a copy the host
    int32_t host_len = (int32_t)(host_end - host_start) + 1;
    char *host = (char *)calloc(host_len, sizeof(char));
    if (!host)
        return strdup(url);

    strncpy(host, host_start, host_len);
    host[host_len - 1] = 0;
    return host;
}

// Find path for a given url
const char *get_url_path(const char *url) {
    // Find the start of the host after the scheme
    const char *host_start = strstr(url, "://");
    if (host_start)
        host_start += 3;
    else
        host_start = url;

    // Find the end of the host
    const char *host_end = strstr(host_start, "/");
    if (!host_end)
        host_end = (char *)host_start + strlen(host_start);

    // Always return slash if no path
    if (*host_end == 0)
        return "/";

    return host_end;
}

// Get the scheme for a given url
char *get_url_scheme(const char *url, const char *fallback) {
    // Find the end of the scheme
    const char *scheme_end = strstr(url, "://");
    if (scheme_end) {
        // Create copy of scheme and return
        size_t scheme_len = (int32_t)(scheme_end - url);
        char *scheme = (char *)calloc(scheme_len + 1, sizeof(char));
        if (scheme) {
            memcpy(scheme, url, scheme_len);
            return scheme;
        }
    }

    // Return default if no scheme found
    if (fallback)
        return strdup(fallback);

    return NULL;
}

// Convert proxy list returned by FindProxyForURL to a list of uris separated by commas.
// The proxy list contains one or more proxies separated by semicolons:
//    returnValue = type host,":",port,[{ ";",returnValue }];
//    type        = "DIRECT" | "PROXY" | "SOCKS" | "HTTP" | "HTTPS" | "SOCKS4" | "SOCKS5"
char *convert_proxy_list_to_uri_list(const char *proxy_list, const char *fallback_scheme) {
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
        // Ignore leading whitespace
        while (*config_start == ' ')
            config_start++;

        // Proxies can be separated by a semi-colon or a whitespace
        config_end = strchr(config_start, ';');
        if (!config_end)
            config_end = config_start + strlen(config_start);
        size_t config_len = (size_t)(config_end - config_start);

        // Find type boundary
        const char *host_start = config_start;
        const char *scheme = fallback_scheme;
        if (!strncasecmp(config_start, "PROXY ", 6)) {
            host_start += 6;
        } else if (!strncasecmp(config_start, "DIRECT", 6)) {
            scheme = "direct";
            host_start += 6;
        } else if (!strncasecmp(config_start, "HTTP ", 5)) {
            scheme = "http";
            host_start += 5;
        } else if (!strncasecmp(config_start, "HTTPS ", 6)) {
            scheme = "https";
            host_start += 6;
        } else if (!strncasecmp(config_start, "SOCKS ", 6)) {
            scheme = "socks";
            host_start += 6;
        } else if (!strncasecmp(config_start, "SOCKS4 ", 7)) {
            scheme = "socks4";
            host_start += 7;
        } else if (!strncasecmp(config_start, "SOCKS5 ", 7)) {
            scheme = "socks5";
            host_start += 7;
        }
        size_t host_len = (size_t)(config_end - host_start);

        // Determine scheme for type PROXY
        if (scheme == NULL) {
            // Parse port from host
            const char *port_start = str_find_len_char(host_start, host_len, ':');
            int32_t port = 80;
            if (port_start) {
                port_start++;
                port = strtoul(port_start, NULL, 0);
            }

            // Use scheme based on port specified
            switch (port) {
                case 443: scheme = "https"; break;
                case 80: scheme = "http"; break;
                case 1080: scheme = "socks"; break;
                default: scheme = "http"; break;
            }
        }

        // Append proxy to uri list
        strncat(uri_list, scheme, max_uri_list - uri_list_len - 1);
        uri_list_len += strlen(scheme);
        strncat(uri_list, "://", max_uri_list - uri_list_len - 1);
        uri_list_len += 3;
        if (host_len > max_uri_list)
            host_len = max_uri_list - 1;
        strncat(uri_list, host_start, host_len);
        uri_list_len += strlen(scheme) + host_len;

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
