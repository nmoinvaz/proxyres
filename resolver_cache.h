#pragma once

// Cache proxy list for url
void proxy_resolver_cache_add(const char *url, const char *list);

// Get cached proxy list for url
const char *proxy_resolver_cache_match(const char *url);

// Initialize cache
bool proxy_resolver_cache_init(void);

// Cleanup cache
bool proxy_resolver_cache_cleanup(void);
