#pragma once

typedef struct proxy_resolver_i_s {
    bool (*get_proxies_for_url)(void *ctx, const char *url);

    bool (*get_list)(void *ctx, char **list);
    bool (*get_error)(void *ctx, int32_t *error);
    bool (*is_pending)(void *ctx);
    bool (*cancel)(void *ctx);

    bool (*set_resolved_callback)(void *ctx, void *user_data, proxy_resolver_resolved_cb callback);

    bool (*create)(void **ctx);
    bool (*delete)(void **ctx);

    bool (*uninit)(void);
} proxy_resolver_i_s;
