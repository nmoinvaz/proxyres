#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool proxy_config_get_auto_discover(void);
char *proxy_config_get_auto_config_url(void);
char *proxy_config_get_proxy(const char *protocol);
char *proxy_config_get_bypass_list(void);

void proxy_config_set_auto_config_url_override(const char *auto_config_url);
void proxy_config_set_proxy_override(const char *proxy);
void proxy_config_set_bypass_list_override(const char *bypass_list);

bool proxy_config_init(void);
bool proxy_config_uninit(void);

#ifdef __cplusplus
}
#endif
