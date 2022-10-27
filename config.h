#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_config_get_auto_discover(void);
char *proxy_config_get_auto_config_url(void);
char *proxy_config_get_proxy(const char *protocol);
char *proxy_config_get_bypass_list(void);

#ifdef __cplusplus
}
#endif
