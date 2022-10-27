#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_config_gnome3_get_auto_discover(void);
char *proxy_config_gnome3_get_auto_config_url(void);
char *proxy_config_gnome3_get_proxy(const char *protocol);
char *proxy_config_gnome3_get_bypass_list(void);

proxy_config_i_s *proxy_config_gnome3_get_interface(void);

#ifdef __cplusplus
}
#endif
