#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_config_gnome2_get_auto_discover(void);
char *proxy_config_gnome2_get_auto_config_url(void);
char *proxy_config_gnome2_get_proxy(const char *scheme);
char *proxy_config_gnome2_get_bypass_list(void);

bool proxy_config_gnome2_init(void);
bool proxy_config_gnome2_uninit(void);

proxy_config_i_s *proxy_config_gnome2_get_interface(void);

#ifdef __cplusplus
}
#endif
