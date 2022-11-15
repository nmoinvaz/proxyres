#pragma once

typedef struct proxy_resolver_i_s {
    bool (*get_proxies_for_url)(void *ctx, const char *url);

    const char *(*get_list)(void *ctx);
    bool (*get_error)(void *ctx, int32_t *error);
    bool (*wait)(void *ctx, int32_t timeout_ms);
    bool (*cancel)(void *ctx);

    void *(*create)(void);
    bool (*delete)(void **ctx);

    bool (*is_async)(void);

    bool (*init)(void);
    bool (*uninit)(void);
} proxy_resolver_i_s;
