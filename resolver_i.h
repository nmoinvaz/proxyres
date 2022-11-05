#pragma once

typedef struct proxy_resolver_i_s {
    bool (*get_proxies_for_url)(void *ctx, const char *url);

    const char *(*get_list)(void *ctx);
    bool (*get_error)(void *ctx, int32_t *error);
    bool (*is_pending)(void *ctx);
    bool (*cancel)(void *ctx);

    bool (*set_resolved_callback)(void *ctx, void *user_data, proxy_resolver_resolved_cb callback);

    void *(*create)(void);
    bool (*delete)(void **ctx);

    bool (*is_async)(void);

    bool (*init)(void);
    bool (*uninit)(void);
} proxy_resolver_i_s;
