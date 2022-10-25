#include "proxyres.h"
#include "proxyres_i.h"

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_resolver_gnome3_get_proxies_for_url(void *ctx, const char *url);

bool proxy_resolver_gnome3_get_list(void *ctx, char **list);
bool proxy_resolver_gnome3_get_error(void *ctx, int32_t *error);
bool proxy_resolver_gnome3_is_pending(void *ctx);
bool proxy_resolver_gnome3_cancel(void *ctx);

bool proxy_resolver_gnome3_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback);

bool proxy_resolver_gnome3_create(void **ctx);
bool proxy_resolver_gnome3_delete(void **ctx);

bool proxy_resolver_gnome3_init(void);
bool proxy_resolver_gnome3_uninit(void);

proxy_resolver_i_s *proxy_resolver_gnome3_get_interface(void);

#ifdef __cplusplus
}
#endif
