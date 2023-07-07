#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <errno.h>

#ifdef _WIN32
#  define strcasecmp _stricmp
#endif

#include "mutex.h"
#include "util.h"

typedef struct resolved_s {
    // Url without path or query
    char *root_url;
    // Proxy list
    char *list;
    // Next cached result
    struct resolved_s *next;
} resolved_s;

typedef struct g_proxy_resolver_cache_s {
    // Cached resolutions list
    resolved_s *results;
    // Resolution list mutex
    void *results_lock;
} g_proxy_resolver_cache_s;

g_proxy_resolver_cache_s g_proxy_resolver_cache;

// Add result to the proxy resolver cache
void proxy_resolver_cache_add(const char *url, const char *list) {
    if (!url || !g_proxy_resolver_cache.results_lock)
        return;

    resolved_s *cache = calloc(1, sizeof(resolved_s));
    if (!cache)
        return;

    // Copy url without path or query
    cache->root_url = get_url_without_path_or_query(url);
    if (!cache->root_url)
        goto add_cleanup;

    // Copy proxy list
    if (list) {
        cache->list = strdup(list);
        if (!cache->list)
            goto add_cleanup;
    }

    // Add to cache
    mutex_lock(g_proxy_resolver_cache.results_lock);
    cache->next = g_proxy_resolver_cache.results;
    g_proxy_resolver_cache.results = cache;
    mutex_unlock(g_proxy_resolver_cache.results_lock);
    return;

add_cleanup:
    free(cache->root_url);
    free(cache);
}

// Find a cached result for the given lookup url
const char *proxy_resolver_cache_match(const char *url) {
    if (!g_proxy_resolver_cache.results_lock)
        return NULL;

    // Use url without a path or query as the key because most systems implement url path stripping for proxies.
    char *root_url = get_url_without_path_or_query(url);
    if (!root_url)
        return NULL;

    mutex_lock(g_proxy_resolver_cache.results_lock);
    resolved_s *cache;
    for (cache = g_proxy_resolver_cache.results; cache; cache = cache->next) {
        if (strcasecmp(cache->root_url, root_url) == 0)
            break;
    }
    mutex_unlock(g_proxy_resolver_cache.results_lock);
    free(root_url);

    if (cache)
        return cache->list;

    return NULL;
}

bool proxy_resolver_cache_init(void) {
    memset(&g_proxy_resolver_cache, 0, sizeof(g_proxy_resolver_cache));
    g_proxy_resolver_cache.results_lock = mutex_create();
    if (!g_proxy_resolver_cache.results_lock)
        return false;
    return true;
}

bool proxy_resolver_cache_cleanup(void) {
    resolved_s *cache = g_proxy_resolver_cache.results;

    while (cache) {
        resolved_s *next_cache = cache->next;

        free(cache->root_url);
        free(cache->list);
        free(cache);

        cache = next_cache;
    }

    if (g_proxy_resolver_cache.results_lock)
        mutex_delete(&g_proxy_resolver_cache.results_lock);
    return true;
}
