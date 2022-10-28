#include <stdint.h>
#include <stdbool.h>

#include <dlfcn.h>
#include <glib.h>
#include <gconf/gconf.h>

#include "config.h"
#include "config_i.h"
#include "config_gnome3.h"

typedef struct g_proxy_config_gnome3_s {
    // GIO module handle
    void *gio_module;
    // GConf settings functions
    GSettings *(*g_settings_new)(const gchar *schema_id);
    gchar *(*g_settings_get_string)(GSettings *settings, const gchar *key);
    gint (*g_settings_get_int)(GSettings *settings, const gchar *key);
    gchar **(*g_settings_get_strv)(GSettings *settings, const gchar *key);
    gboolean (*g_settings_get_boolean)(GSettings *settings, const gchar *key);
    // Glib module handle
    void *glib_module;
    // Glib memory functions
    void (*g_free)(gpointer Mem);
} g_proxy_config_gnome3_s;

g_proxy_config_gnome3_s g_proxy_config_gnome3;

bool proxy_config_gnome3_get_auto_discover(void) {
    char *mode = NULL;
    bool auto_discover = false;

    mode = g_proxy_config_gnome3.g_settings_get_string(g_proxy_config_gnome3.settings_default, "mode");
    if (mode != NULL) {
        auto_discover = strcmp(mode, "auto");
        g_proxy_config_gnome3.free(mode);
    }
    return auto_discover;
}

char *proxy_config_gnome3_get_auto_config_url(void) {
    char *auto_config_url = NULL;

    url = g_proxy_config_gnome3.g_settings_get_string(g_proxy_config_gnome3.settings_default, "autoconfig_url", NULL);
    if (url != NULL) {
        if (*url != 0)
            auto_config_url = strdup(url);
        g_proxy_config_gnome3.free(url);
    }

    return auto_config_url;
}

char *proxy_config_gnome3_get_proxy(const char *protocol) {
    char host_key[128];
    char port_key[128];
    char *host = NULL;
    uint32_t port = 0;
    char *proxy = NULL;

    snprintf(host_key, sizeof(host_key), "%s.host", protocol);
    snprintf(port_key, sizeof(port_key), "%s.port", protocol);

    host = g_proxy_config_gnome3.g_settings_get_string(g_proxy_config_gnome3.gconf_default, host_key);
    if (host) {
        // Allocate space for host:port
        int32_t max_proxy = strlen(host) + 32;
        proxy = (char *)malloc(max_proxy);
        if (proxy) {
            port = g_proxy_config_gnome3.g_settings_get_int(g_proxy_config_gnome3.gconf_default, port_key);
            if (port == 0)
                snprintf(proxy, max_proxy, "%s", host);
            else
                snprintf(proxy, max_proxy, "%s:%u", host, port);
        }

        g_proxy_config_gnome3.free(host);
    }
    return proxy;
}

char *proxy_config_gnome3_get_bypass_list(void) {
    GSList *hosts = NULL;
    g_slist_for_each_bypass_s enum_bypass = {0};
    char *bypass_list = NULL;

    hosts = g_proxy_config_gnome3.g_settings_get_strv(g_proxy_config_gnome3.gconf_default, "ignore_hosts");
    if (hosts != NULL) {
        int32_t max_value = 0;
        // Enumerate the list to get the size of the bypass list
        for (int32_t i = 0; hosts[i] != NULL; i++)
            max_value = strlen(hosts[i]) + 2;

        // Allocate space for the bypass list
        bypass_list = calloc(max_value, sizeof(char));
        if (bypass_list) {
            // Enumerate hosts and copy them to the bypass list
            for (int32_t i = 0; hosts[i] != NULL; i++) {
                int32_t bypass_list_len = strlen(bypass_list);
                snprintf(bypass_list + bypass_list_len, max_value - bypass_list_len, "%s,", hosts[i]);
            }

            // Remove the last comma
            int32_t bypass_list_len = strlen(bypass_list);
            if (bypass_list_len > 2 && bypass_list[bypass_list_len - 2] == ',')
                bypass_list[bypass_list_len - 1] = 0;
        }

        g_proxy_config_gnome3.g_slist_free_full(hosts, g_proxy_config_gnome3.free);
    }

    return Result;
}

bool proxy_config_gnome3_init(void) {
    g_proxy_resolver_gnome3.gio_module = dlopen("libgio-2.0.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!g_proxy_resolver_gnome3.gio_module)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.glib_module = dlopen("libglib-2.0.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!g_proxy_resolver_gnome3.glib_module)
        goto gnome2_init_error;

    // Glib functions
    g_proxy_resolver_gnome3.free = dlopen(g_proxy_config_gnome2.glib_module, "g_free");
    if (!g_proxy_resolver_gnome3.free)
        goto gnome2_init_error;

    // Glib functions
    g_proxy_config_gnome3.g_settings_new = dlopen(g_proxy_config_gnome3.gio_module, "g_settings_new");
    if (!g_proxy_config_gnome3.g_settings_new)
        goto gnome3_init_error;
    g_proxy_config_gnome3.g_settings_get_string = dlsym(g_proxy_config_gnome3.gio_module, "g_settings_get_string");
    if (!g_proxy_config_gnome3.g_settings_get_string)
        goto gnome3_init_error;
    g_proxy_config_gnome3.g_settings_get_int = dlsym(g_proxy_config_gnome3.gio_module, "g_settings_get_int");
    if (!g_proxy_config_gnome3.g_settings_get_int)
        goto gnome3_init_error;
    g_proxy_config_gnome3.g_settings_get_strv = dlsym(g_proxy_config_gnome3.gio_module, "g_settings_get_strv");
    if (!g_proxy_config_gnome3.g_settings_get_strv)
        goto gnome3_init_error;
    g_proxy_config_gnome3.g_settings_get_boolean = dlsym(g_proxy_config_gnome3.gio_module, "g_settings_get_boolean");
    if (!g_proxy_config_gnome3.g_settings_get_boolean)
        goto gnome3_init_error;

    g_proxy_config_gnome3.settings_default = g_proxy_config_gnome3.settings_new("org.gnome.system.proxy");
    return true;

gnome3_init_error:
    proxy_config_gnome3_uninit();
    return false;
}

bool proxy_config_gnome3_uninit(void) {
    // g_object_unref settings

    if (g_proxy_config_gnome3.gio_module)
        dlclose(g_proxy_config_gnome3.gio_module);
    if (g_proxy_config_gnome3.glib_module)
        dlclose(g_proxy_config_gnome3.glib_module);

    memset(&g_proxy_config_gnome3, 0, sizeof(g_proxy_config_gnome3));
    return true;
}

proxy_config_i_s *proxy_config_gnome3_get_interface(void) {
    static proxy_config_i_s proxy_config_gnome3_i = {proxy_config_gnome3_get_auto_discover,
                                                     proxy_config_gnome3_get_auto_config_url,
                                                     proxy_config_gnome3_get_proxy,
                                                     proxy_config_gnome3_get_bypass_list,
                                                     proxy_config_gnome3_init,
                                                     proxy_config_gnome3_uninit};
    return &proxy_config_gnome3_i;
}
