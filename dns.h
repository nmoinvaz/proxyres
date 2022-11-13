#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Resolve a DNS name to an IP address
char *dns_resolve(const char *host, int32_t *error);

#ifdef __cplusplus
}
#endif
