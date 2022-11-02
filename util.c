#include <stdint.h>
#include <stdbool.h>
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

// Read a value from an ini config file given the section and key
char *get_config_value(const char *config, const char *section, const char *key) {
    size_t max_config = strlen(config);
    int32_t line_len = 0;
    const char *line_start = config;
    bool in_section = true;

    // Read ini file until we find the section and key
    do {
        // Find end of line
        const char *line_end = strchr(line_start, '\n');
        if (!line_end)
            line_end = line_start + strlen(line_start);
        line_len = (int32_t)(line_end - line_start);

        // Check for the key if we are already in the section
        if (in_section) {
            const char *key_start = line_start;
            const char *key_end = strchr(key_start, '=');
            if (key_end) {
                int32_t key_len = (int32_t)(key_end - key_start);
                if (strncmp(key_start, key, key_len) == 0) {
                    // Found key, now make a copy of the value
                    int32_t value_len = line_len - key_len - 1;
                    if (value_len >= 0) {
                        char *value = (char *)calloc(value_len + 1, sizeof(char));
                        if (value) {
                            strncpy(value, key_end + 1, value_len);
                            value[value_len] = 0;
                        }
                        return value;
                    }
                }
            }
        }

        // Check to see if we are in the right section
        if (line_len > 2 && line_start[0] == '[' && line_end[-1] == ']')
            in_section = strncmp(line_start + 1, section, line_len - 2) == 0;

        // Continue to the next line
        line_start = line_end + 1;
    } while (line_start < config + max_config);

    return NULL;
}
