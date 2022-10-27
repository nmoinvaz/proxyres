#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_config_mac_get_auto_discover(void);
char *proxy_config_mac_get_auto_config_url(void);
char *proxy_config_mac_get_proxy(const char *protocol);
char *proxy_config_mac_get_bypass_list(void);

bool proxy_config_mac_init(void);
bool proxy_config_mac_uninit(void);

proxy_config_i_s *proxy_config_mac_get_interface(void);

#ifdef __cplusplus
}
#endif
