#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_config_get_auto_discover(void);
bool proxy_config_get_auto_config_url(char *url, int32_t max_url);
bool proxy_config_get_proxy(char *protocol, char *proxy, int32_t max_proxy);
bool proxy_config_get_bypass_list(char *bypass_list, int32_t max_bypass_list);

#ifdef __cplusplus
}
#endif
