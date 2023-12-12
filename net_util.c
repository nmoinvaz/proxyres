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
#  include <arpa/inet.h>
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

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(host, NULL, &hints, &address_info);
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

            // Copy address name into numeric host
            err = getnameinfo(address->ai_addr, (socklen_t)address->ai_addrlen, ai_string + ai_string_len,
                              (uint32_t)(max_ai_string - ai_string_len), NULL, 0, NI_NUMERICHOST);
            if (err != 0) {
                if (ai_string_len < max_ai_string)
                    ai_string[ai_string_len] = 0;
                continue;
            }

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

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
// Backwards compatible inet_pton for Windows XP
int32_t inet_pton(int32_t af, const char *src, void *dst) {
    struct sockaddr_storage sock_storage;
    int32_t size = sizeof(sockaddr_storage);
    char src_copy[INET6_ADDRSTRLEN + 1];

    memset(&sock_storage, 0, sizeof(sock_storage));
    strncpy(src_copy, src, INET6_ADDRSTRLEN + 1);
    src_copy[INET6_ADDRSTRLEN] = 0;

    if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&sock_storage, &size) == 0) {
        switch (af) {
        case AF_INET:
            *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
            return 1;
        case AF_INET6:
            *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
            return 1;
        }
    }
    return 0;
}
#endif

// Check if the ipv4 address matches the cidr notation range
bool is_ipv4_in_cidr_range(const char *ip, const char *cidr) {
    if (!ip || !cidr)
        return false;

    // Convert ip from text to binary
    struct in_addr ip_addr;
    if (!inet_pton(AF_INET, ip, &ip_addr))
        return false;

    // Parse cidr notation
    char *cidr_ip = strdup(cidr);
    char *cidr_prefix = strchr(cidr_ip, '/');
    if (!cidr_prefix) {
        free(cidr_ip);
        return false;
    }
    *cidr_prefix = 0;
    cidr_prefix++;

    // Parse cidr prefix
    int32_t prefix = atoi(cidr_prefix);
    if (prefix < 0 || prefix > 32) {
        free(cidr_ip);
        return false;
    }

    // Convert cidr ip from text to binary
    struct in_addr cidr_addr;
    if (!inet_pton(AF_INET, cidr_ip, &cidr_addr)) {
        free(cidr_ip);
        return false;
    }
    free(cidr_ip);

    // Check if ip address is in cidr range
    uint32_t ip_int = ntohl(ip_addr.s_addr);
    uint32_t cidr_int = ntohl(cidr_addr.s_addr);
    uint32_t mask = prefix >= 32 ? 0xFFFFFFFFu : ~(0xFFFFFFFFu >> prefix);

    return (ip_int & mask) == (cidr_int & mask);
}

// Check if the ipv6 address matches the cidr notation range
bool is_ipv6_in_cidr_range(const char *ip, const char *cidr) {
    if (!ip || !cidr)
        return false;

    // Convert ip from text to binary
    struct in6_addr ip_addr;
    if (!inet_pton(AF_INET6, ip, &ip_addr))
        return false;

    // Parse cidr notation
    char *cidr_ip = strdup(cidr);
    char *cidr_prefix = strchr(cidr_ip, '/');
    if (!cidr_prefix) {
        free(cidr_ip);
        return false;
    }
    *cidr_prefix = 0;
    cidr_prefix++;

    // Parse cidr prefix
    int32_t prefix = atoi(cidr_prefix);
    if (prefix < 0 || prefix > 128) {
        free(cidr_ip);
        return false;
    }

    // Convert cidr ip from text to binary
    struct in6_addr cidr_addr;
    if (!inet_pton(AF_INET6, cidr_ip, &cidr_addr)) {
        free(cidr_ip);
        return false;
    }
    free(cidr_ip);

    // Check if ip address is in cidr range
    uint8_t *ip_data = (uint8_t *)&ip_addr.s6_addr;
    uint8_t *cidr_data = (uint8_t *)&cidr_addr.s6_addr;

    // Compare leading bytes of address
    int32_t check_bytes = prefix / 8;
    if (check_bytes) {
        if (memcmp(ip_data, cidr_data, check_bytes))
            return false;
    }

    // Check remaining bits of address
    int32_t check_bits = prefix & 0x07;
    if (!check_bits)
        return true;

    uint8_t mask = (0xff << (8 - check_bits));
    return ((ip_data[check_bytes] ^ cidr_data[check_bytes]) & mask) == 0;
}
