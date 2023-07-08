#pragma once

typedef struct proxy_resolver_i_s {
    bool (*discover_proxies_for_url)(void *ctx, const char *url);

    const char *(*get_list)(void *ctx);
    int32_t (*get_error)(void *ctx);
    bool (*wait)(void *ctx, int32_t timeout_ms);
    bool (*cancel)(void *ctx);

    void *(*create)(void);
    bool (*delete)(void **ctx);

    bool discover_is_async;
    bool discover_uses_system_config;

    bool (*global_init)(void);
    bool (*global_cleanup)(void);
} proxy_resolver_i_s;
