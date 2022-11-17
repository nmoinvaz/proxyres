#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#define strcasecmp  _stricmp
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

// Compare a string using wildcard pattern
bool str_wildcard_match(const char *str, const char *pattern, bool ignore_case) {
    while (*str) {
        switch (*pattern) {
        case '*':
            if (pattern[1] == 0)
                return true;

            while (*str) {
                if (str_wildcard_match(str, pattern + 1, ignore_case))
                    return true;
                str++;
            }

            return false;

        default:
            if (ignore_case) {
                if (tolower(*str) != tolower(*pattern))
                    return false;
            } else {
                if (*str != *pattern)
                    return false;
            }
            break;
        }

        str++;
        pattern++;
    }

    if (*pattern && *pattern != '*')
        return false;

    return true;
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
char *get_url_scheme(const char *url, const char *default_scheme) {
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
    if (default_scheme)
        return strdup(default_scheme);

    return NULL;
}

// Create url from host with port
char *get_url_from_host(const char *scheme, const char *host) {
    // Get scheme in case we are passed a url
    char *url_scheme = get_url_scheme(scheme, "http");

    // Create buffer to store and return url
    size_t max_url = strlen(host) + strlen(url_scheme) + 16;
    char *url = (char *)calloc(max_url, sizeof(char));
    if (!url)
        return NULL;

    // Construct url with scheme and host
    snprintf(url, max_url, "%s://%s", url_scheme, host);

    // Append port if it does not exist
    if (strchr(host, ':') == NULL) {
        size_t url_len = strlen(url);
        snprintf(url + url_len, max_url - url_len, ":%d", get_scheme_default_port(url_scheme));
    }

    return url;
}

// Get port from host
int32_t get_host_port(const char *host, size_t host_len, int32_t default_port) {
    int32_t port = default_port;
    const char *port_start = str_find_len_char(host, host_len, ':');
    if (port_start) {
        port_start++;
        port = strtoul(port_start, NULL, 0);
    }
    return port;
}

// Use scheme based on port specified
const char *get_port_scheme(int32_t port, const char *default_scheme) {
    switch (port) {
    case 80:
        return "http";
    case 443:
        return "https";
    case 1080:
        return "socks";
    case 21:
        return "ftp";
    }
    return default_scheme;
}

// Get default port for a scheme
int32_t get_scheme_default_port(const char *scheme) {
    // Use scheme based on port specified
    if (!strcasecmp(scheme, "http"))
        return 80;
    if (!strcasecmp(scheme, "https"))
        return 443;
    if (!strcasecmp(scheme, "socks"))
        return 1080;
    if (!strcasecmp(scheme, "ftp"))
        return 21;
    return -1;
}

// Convert proxy list returned by FindProxyForURL to a list of uris separated by commas.
// The proxy list contains one or more proxies separated by semicolons:
//    returnValue = type host,":",port,[{ ";",returnValue }];
//    type        = "DIRECT" | "PROXY" | "SOCKS" | "HTTP" | "HTTPS" | "SOCKS4" | "SOCKS5"
char *convert_proxy_list_to_uri_list(const char *proxy_list, const char *default_scheme) {
    if (!proxy_list)
        return NULL;

    size_t proxy_list_len = strlen(proxy_list);
    const char *proxy_list_end = proxy_list + proxy_list_len;

    size_t uri_list_len = 0;
    size_t max_uri_list = proxy_list_len + 128;  // Extra space for scheme separators
    char *uri_list = (char *)calloc(max_uri_list, sizeof(char));
    if (!uri_list)
        return NULL;

    // Enumerate each proxy in the proxy list.
    const char *config_start = proxy_list;
    const char *config_end = NULL;

    do {
        // Ignore leading whitespace
        while (*config_start == ' ')
            config_start++;

        // Proxies can be separated by a semi-colon
        config_end = strchr(config_start, ';');
        if (!config_end)
            config_end = config_start + strlen(config_start);

        // Find type boundary
        const char *host_start = config_start;
        const char *scheme = default_scheme;
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
            int32_t port = get_host_port(host_start, host_len, 80);

            // Use scheme based on port
            scheme = get_port_scheme(port, "http");
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

bool should_bypass_proxy(const char *url, const char *bypass_list) {
    char bypass_rule[HOST_MAX] = {0};
    bool is_local = false;
    bool is_simple = false;
    bool should_bypass = false;

    if (!url || !bypass_list)
        return true;

    // Chromium documentation for proxy bypass rules:
    // https://chromium.googlesource.com/chromium/src/+/HEAD/net/docs/proxy.md#Proxy-bypass-rules

    // Parse host without port for url
    char *host = get_url_host(url);
    if (!host)
        return true;
    char *port = strchr(host, ':');
    if (port)
        *port = 0;

    // Check for localhost address
    if (strcmp(host, "127.0.0.1") == 0 || strcasecmp(host, "localhost") == 0)
        is_local = true;

    // Check for simple hostnames
    if (strchr(host, '.') == NULL)
        is_simple = true;

    // By default don't allow localhost urls to go through proxy
    if (is_local)
        should_bypass = true;

    // Enumerate through each bypass expression in the bypass list
    const char *bypass_list_end = bypass_list + strlen(bypass_list);
    const char *rule_start = bypass_list;
    const char *rule_end = NULL;

    do {
        // Ignore leading whitespace
        while (*rule_start == ' ')
            rule_start++;

        // Rules can be separated by a comma
        rule_end = strchr(rule_start, ',');
        if (!rule_end)
            rule_end = rule_start + strlen(rule_start);
        size_t rule_len = (size_t)(rule_end - rule_start);
        if (rule_len > sizeof(bypass_rule))
            rule_len = sizeof(bypass_rule) - 1;
        strncat(bypass_rule, rule_start, rule_len);

        // If the rule matches hostname of url then bypass proxy
        if (str_wildcard_match(host, bypass_rule, true))
            should_bypass = true;

        // If the rule is <local> then allow all simple hostnames to bypass proxy
        if (is_simple && strcasecmp(bypass_rule, "<local>") == 0)
            should_bypass = true;
        // If the rule is <-loopback> then allow localhost urls to go through proxy
        if (is_local && strcasecmp(bypass_rule, "<-loopback>") == 0)
            should_bypass = false;

        rule_start = rule_end + 1;
    } while (rule_end < bypass_list_end);

    return should_bypass;
}
