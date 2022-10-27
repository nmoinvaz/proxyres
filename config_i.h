#pragma once

typedef struct proxy_config_i_s {
    bool (*auto_discover)(void);
    char *(*get_auto_config_url)(void);
    char *(*get_proxy)(const char *protocol);
    char *(*get_bypass_list)(void);

    bool (*init)(void);
    bool (*uninit)(void);
} proxy_config_i_s;
