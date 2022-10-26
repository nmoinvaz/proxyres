#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_config_win_get_auto_discover(void);
const char *proxy_config_win_get_auto_config_url(void);
const char *proxy_config_win_get_proxy(const char *protocol);
const char *proxy_config_win_get_bypass_list(void);

#ifdef __cplusplus
}
#endif
