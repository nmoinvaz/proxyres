#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>

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
char *str_find_first_char(char *str, char *chars) {
    while (*str) {
        char *c = chars;
        while (*c) {
            if (*str == *c)
                return str;
            c++;
        }
    }
    return NULL;
}

// Find character in string up to max length
char *str_find_char_safe(char *str, int32_t max_str, char c) {
    while (max_str && *str) {
        if (*str == c)
            return str;
        str++;
        max_str--;
    }
    return NULL;
}

// Resolve a hostname to an IP address
char *dns_resolve(const char *host, int32_t *error) {
    char name[256] = {0};
    struct addrinfo hints = {0};
    struct addrinfo *address_info = NULL;
    struct addrinfo *next_address = NULL;
    char *ip_addr = NULL;
    int32_t err = 0;

    if (host == NULL) {
        err = gethostname(name, sizeof(name));
        if (err != 0)
            goto my_ip_address_error;
    } else {
        strncpy(name, host, sizeof(name));
    }
    name[sizeof(name) - 1] = 0;

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    err = getaddrinfo(name, NULL, &hints, &address_info);
    if (err != EAI_NONAME) {
        if (err != 0)
            goto my_ip_address_error;

        // Name is already an IP address
        freeaddrinfo(address_info);
        return strdup(name);
    }

    hints.ai_flags = 0;
    err = getaddrinfo(name, NULL, &hints, &address_info);
    if (err != 0)
        goto my_ip_address_error;

    // Convert IP address to string
    next_address = address_info;
    while (next_address->ai_family != AF_INET) {
        next_address = next_address->ai_next;
        // Name not resolved
        if (next_address == NULL)
            goto my_ip_address_error;
    }

    ip_addr = (char *)calloc(1, INET6_ADDRSTRLEN);
    if (!ip_addr)
        goto my_ip_address_error;

    err = getnameinfo(next_address->ai_addr, (socklen_t)next_address->ai_addrlen, ip_addr, INET6_ADDRSTRLEN, NULL, 0,
                      NI_NUMERICHOST);

    if (err != 0)
        goto my_ip_address_error;

    return ip_addr;

my_ip_address_error:

    free(ip_addr);

    if (address_info)
        freeaddrinfo(address_info);

    if (error != NULL)
        *error = err;

    return NULL;
}

// Find host for a given url
char *parse_url_host(const char *url) {
    // Find the start of the host after the schema
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
