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

// Resolve a DNS name to an IP address
char *dns_resolve(const char *host, int32_t *error) {
    char name[HOST_MAX] = {0};
    struct addrinfo hints = {0};
    struct addrinfo *address_info = NULL;
    struct addrinfo *next_address = NULL;
    char *ip_addr = NULL;
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

    // Get AF_INET address from address list
    next_address = address_info;
    while (next_address->ai_family != AF_INET) {
        next_address = next_address->ai_next;
        // Name not resolved
        if (!next_address)
            goto dns_resolve_error;
    }

    // Convert IP address to string
    ip_addr = (char *)calloc(1, INET6_ADDRSTRLEN);
    if (!ip_addr)
        goto dns_resolve_error;

    err = getnameinfo(next_address->ai_addr, (socklen_t)next_address->ai_addrlen, ip_addr, INET6_ADDRSTRLEN, NULL, 0,
                      NI_NUMERICHOST);

    if (err != 0)
        goto dns_resolve_error;

    return ip_addr;

dns_resolve_error:

    free(ip_addr);

    if (address_info)
        freeaddrinfo(address_info);

    if (error != NULL)
        *error = err;

    return NULL;
}
