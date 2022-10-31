#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

#include "config.h"
#include "config_i.h"
#include "config_kde.h"
#include "util_linux.h"

typedef struct g_proxy_config_kde_s {
    // User config file
    char *config;
} g_proxy_config_kde_s;

g_proxy_config_kde_s g_proxy_config_kde;

enum proxy_type_enum {
    PROXY_TYPE_NONE,
    PROXY_TYPE_FIXED,
    PROXY_TYPE_PAC,
    PROXY_TYPE_WPAD,
    PROXY_TYPE_ENV
};

static char *get_ini_setting(const char *section, const char *key) {
    int32_t max_config = strlen(g_proxy_config_kde.config);
    int32_t line_len = 0;
    const char *line_start = g_proxy_config_kde.config;
    bool in_section = true;

    // Read config file until we find the section and key
    do {
        // Find end of line
        const char *line_end = strchr(line_start, '\n');
        if (!line_end)
            line_end = line_start + strlen(line_start);
        line_len = (int32_t)(line_end - line_start);

        // Check for the key if we are already in the section
        if (in_section) {
            const char *key_start = line_start;
            const char *key_end = strchr(key_start, '=');
            if (key_end) {
                int32_t key_len = (int32_t)(key_end - key_start);
                if (strncmp(key_start, key, key_len) == 0) {
                    // Found key, now make a copy of the value
                    int32_t value_len = line_len - key_len - 1;
                    if (value_len) {
                        char *value = (char *)calloc(value_len + 1, sizeof(char));
                        strncpy(value, key_end + 1, value_len);
                        value[value_len] = 0;
                        return value;
                    }
                }
            }
        }

        // Check to see if we are in the right section
        if (line_len > 2 && line_start[0] == '[' && line_end[-1] == ']')
            in_section = strncmp(line_start + 1, section, line_len - 2) == 0;

        // Continue to the next line
        line_start = line_end + 1;
    } while (line_start < g_proxy_config_kde.config + max_config);

    return NULL;
}

static bool check_proxy_type(int type) {
    char *proxy_type = get_ini_setting("Proxy Settings", "ProxyType");
    if (!proxy_type)
        return false;
    bool is_equal = atoi(proxy_type) == type;
    free(proxy_type);
    return is_equal;
}

bool proxy_config_kde_get_auto_discover(void) {
    return check_proxy_type(PROXY_TYPE_WPAD);
}

char *proxy_config_kde_get_auto_config_url(void) {
    if (!check_proxy_type(PROXY_TYPE_PAC))
        return NULL;
    return get_ini_setting("Proxy Settings", "Proxy Config Script");
}

char *proxy_config_kde_get_proxy(const char *protocol) {
    if (!protocol || !check_proxy_type(PROXY_TYPE_FIXED))
        return NULL;
    // Construct key name to search for in config
    int32_t protocol_len = strlen(protocol);
    int32_t max_key = protocol_len + 8;
    char *key = (char *)calloc(max_key, sizeof(char));
    if (!key)
        return NULL;

    strncpy(key, protocol, max_key);

    // Protocol should be all lowercase
    for (int32_t i = 0; i < protocol_len; i++)
        key[i] = tolower(key[i]);

    // Append "Proxy" to the end of the key
    strncat(key, "Proxy", max_key - protocol_len - 1);
    key[max_key - 1] = 0;

    char *proxy = get_ini_setting("Proxy Settings", key);
    free(key);
    return proxy;
}

char *proxy_config_kde_get_bypass_list(void) {
    return get_ini_setting("Proxy Settings", "NoProxyFor");
}

bool proxy_config_kde_init(void) {
    char user_home_path[PATH_MAX];
    char config_path[PATH_MAX];
    int fd = 0;

    // Get the user's home directory
    const char *home_env_var = getenv("KDEHOME");
    if (home_env_var) {
        strncpy(user_home_path, home_env_var, sizeof(user_home_path));
    } else {
        struct passwd *pw = getpwuid(getuid());
        if (pw == NULL)
            return false;

        strncpy(user_home_path, pw->pw_dir, sizeof(user_home_path));
    }

    user_home_path[sizeof(user_home_path) - 1] = 0;

    // Remove trailing slash
    int32_t user_home_path_len = strlen(user_home_path);
    if (user_home_path[user_home_path_len - 1] == '/')
        user_home_path[user_home_path_len - 1] = 0;

    // Get config file path based on desktop environment
    int32_t desktop_env = get_desktop_env();
    switch (desktop_env) {
    case DESKTOP_ENV_KDE3:
        snprintf(config_path, sizeof(config_path), "%s/.kde/share/config/kioslaverc", user_home_path);
        break;
    case DESKTOP_ENV_KDE4:
        snprintf(config_path, sizeof(config_path), "%s/.kde4/share/config/kioslaverc", user_home_path);
        break;
    case DESKTOP_ENV_KDE5:
    default:
        snprintf(config_path, sizeof(config_path), "%s/.config/kioslaverc", user_home_path);
        break;
    }

    // Check to see if config file exists
    if (access(config_path, F_OK) == -1)
        return false;

    // Open user config file
    fd = open(config_path, O_RDONLY);
    if (fd == -1)
        return false;

    // Create buffer to store config
    int config_size = lseek(fd, 0, SEEK_END);
    g_proxy_config_kde.config = (char *)calloc(config_size + 1, sizeof(char));
    if (!g_proxy_config_kde.config)
        goto kde_init_error;

    // Read config file into buffer
    lseek(fd, 0, SEEK_SET);
    if (read(fd, g_proxy_config_kde.config, config_size) != config_size)
        goto kde_init_error;

    // Use proxy_config_env instead
    if (check_proxy_type(PROXY_TYPE_ENV))
        goto kde_init_error;

    goto kde_ini_cleanup;

kde_init_error:
    proxy_config_kde_uninit();

kde_ini_cleanup:
    if (fd)
        close(fd);

    return g_proxy_config_kde.config;
}

bool proxy_config_kde_uninit(void) {
    free(g_proxy_config_kde.config);

    memset(&g_proxy_config_kde, 0, sizeof(g_proxy_config_kde));
    return true;
}

proxy_config_i_s *proxy_config_kde_get_interface(void) {
    static proxy_config_i_s proxy_config_kde_i = {proxy_config_kde_get_auto_discover,
                                                  proxy_config_kde_get_auto_config_url,
                                                  proxy_config_kde_get_proxy,
                                                  proxy_config_kde_get_bypass_list,
                                                  proxy_config_kde_init,
                                                  proxy_config_kde_uninit};
    return &proxy_config_kde_i;
}

#ifdef __cplusplus
}
#endif
