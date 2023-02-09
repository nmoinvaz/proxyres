#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Resolve a host name to it an IPv4 address
char *dns_resolve(const char *host, int32_t *error);

// Resolve a host name to its addresses
char *dns_resolve_ex(const char *host, int32_t *error);

// Check if the ipv4 address matches the cidr notation range
bool is_ipv4_in_cidr_range(const char *ip, const char *cidr);

// Check if the ipv6 address matches the cidr notation range
bool is_ipv6_in_cidr_range(const char *ip, const char *cidr);

#ifdef __cplusplus
}
#endif
