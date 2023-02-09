#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Resolve a host name to it an IPv4 address
char *dns_resolve(const char *host, int32_t *error);

// Resolve a host name to its addresses
char *dns_resolve_ex(const char *host, int32_t *error);

#ifdef __cplusplus
}
#endif
