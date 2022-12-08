#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netdb.h>
#  include <unistd.h>
#endif

#include "util.h"

// Resolve a host name to its addresses with a filter and custom separator
static char *dns_resolve_filter(const char *host, int32_t family, uint8_t max_addrs, int32_t *error) {
    char name[HOST_MAX] = {0};
    struct addrinfo hints = {0};
    struct addrinfo *address_info = NULL;
    struct addrinfo *address = NULL;
    char *ai_string = NULL;
    size_t ai_string_len = 0;
    int32_t err = 0;

    // If no host supplied, then use local machine name
    if (!host) {
        err = gethostname(name, sizeof(name));
        if (err != 0)
            goto dns_resolve_error;
    } else {
        // Otherwise copy the host provided
        strncat(name, host, sizeof(name) - 1);
    }

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    err = getaddrinfo(name, NULL, &hints, &address_info);
    if (err != EAI_NONAME) {
        if (err != 0)
            goto dns_resolve_error;

        // Name is already an IP address
        freeaddrinfo(address_info);
        return strdup(name);
    }

    hints.ai_flags = 0;
    err = getaddrinfo(name, NULL, &hints, &address_info);
    if (err != 0)
        goto dns_resolve_error;

    // Calculate the length of the return string
    size_t max_ai_string = 1;
    address = address_info;
    while (address) {
        // Use different length depending on the address type
        if (address->ai_family == AF_INET)
            max_ai_string += INET_ADDRSTRLEN;
        else
            max_ai_string += INET6_ADDRSTRLEN;

        // Add room for semi-colon separator
        max_ai_string++;
        address = address->ai_next;
    }

    // Allocate buffer for the return string
    ai_string = (char *)calloc(1, max_ai_string);
    if (!ai_string)
        goto dns_resolve_error;

    // Enumerate each address
    address = address_info;
    while (address && max_addrs) {
        // Only copy addresses that match the family filter
        if (family == AF_UNSPEC || address->ai_family == family) {
            // Ensure there is room to copy something into return string buffer
            if (ai_string_len >= max_ai_string)
                break;

            // Copy address name into return string
            err = getnameinfo(address->ai_addr, (socklen_t)address->ai_addrlen, ai_string + ai_string_len,
                              (uint32_t)(max_ai_string - ai_string_len), NULL, 0, NI_NUMERICHOST);
            if (err != 0)
                goto dns_resolve_error;

            max_addrs--;

            // Append semi-colon separator
            ai_string_len = strlen(ai_string);
            if (max_addrs && address->ai_next && ai_string_len + 1 < max_ai_string) {
                ai_string[ai_string_len++] = ';';
                ai_string[ai_string_len] = 0;
            }
        }

        address = address->ai_next;
    }

    if (err != 0)
        goto dns_resolve_error;

    return ai_string;

dns_resolve_error:

    free(ai_string);

    if (address_info)
        freeaddrinfo(address_info);

    if (error != NULL)
        *error = err;

    return NULL;
}

// Resolve a host name to it an IPv4 address
char *dns_resolve(const char *host, int32_t *error) {
    return dns_resolve_filter(host, AF_INET, 1, error);
}

// Resolve a host name to its addresses
char *dns_resolve_ex(const char *host, int32_t *error) {
    return dns_resolve_filter(host, AF_UNSPEC, UINT8_MAX, error);
}
