#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "resolver.h"

static void print_proxy_config(void) {
    printf("Proxy configuration\n");

    bool auto_discover = proxy_config_get_auto_discover();
    printf("  Auto discover: %s\n", auto_discover ? "enabled" : "disabled");

    char *auto_config_url = proxy_config_get_auto_config_url();
    if (auto_config_url) {
        printf("  Auto Config Url: %s\n", auto_config_url);
        free(auto_config_url);
    } else {
        printf("  Auto Config Url: not set\n");
    }

    char *proxy = proxy_config_get_proxy("http");
    if (proxy) {
        printf("  Proxy: %s\n", proxy);
        free(proxy);
    } else {
        printf("  Proxy: not set\n");
    }

    char *bypass_list = proxy_config_get_bypass_list();
    if (bypass_list) {
        printf("  Proxy Bypass: %s\n", bypass_list);
        free(bypass_list);
    } else {
        printf("  Proxy Bypass: not set\n");
    }
}
#include <windows.h>
static void resolve_proxy_for_url(const char *url) {
    void *proxy_resolver;
    proxy_resolver_create(&proxy_resolver);
    printf("Resolving proxy for %s\n", url);
    if (proxy_resolver_get_proxies_for_url(proxy_resolver, url)) {
        while (proxy_resolver_is_pending(proxy_resolver)) {
            Sleep(10);
        }
        char *list;
        printf("  Proxy: %s\n", proxy_resolver_get_list(proxy_resolver, &list) ? list : "DIRECT");
    }
    proxy_resolver_delete(&proxy_resolver);
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("config  - dumps all proxy configuration values\n");
        printf("resolve - resolves proxy for urls\n");
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "config") == 0) {
        print_proxy_config();
    } else if (strcmp(cmd, "resolve") == 0) {
        proxy_resolver_init();

        for (int arg = 2; arg < argc; arg++)
            resolve_proxy_for_url(argv[arg]);

        proxy_resolver_uninit();
    }
    return 0;
}