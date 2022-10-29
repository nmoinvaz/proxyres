#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include "proxyres.h"

#ifdef _WIN32
#  include <windows.h>
#  define sleep Sleep
#else
#  include <unistd.h>
#endif

static void print_proxy_config(void) {
    printf("Proxy configuration\n");

    bool auto_discover = proxy_config_get_auto_discover();
    printf("  Auto discover: %s\n", auto_discover ? "enabled" : "disabled");

    char *auto_config_url = proxy_config_get_auto_config_url();
    if (auto_config_url) {
        printf("  Auto config url: %s\n", auto_config_url);
        free(auto_config_url);
    } else {
        printf("  Auto config url: not set\n");
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
        printf("  Proxy bypass: %s\n", bypass_list);
        free(bypass_list);
    } else {
        printf("  Proxy bypass: not set\n");
    }
}

static void resolve_proxy_for_url(const char *url) {
    void *proxy_resolver;

    printf("Resolving proxy for %s\n", url);

    proxy_resolver_create(&proxy_resolver);
    if (proxy_resolver_get_proxies_for_url(proxy_resolver, url)) {
        while (proxy_resolver_is_pending(proxy_resolver)) {
            sleep(10);
        }
        char *list;
        printf("  Proxy: %s\n", proxy_resolver_get_list(proxy_resolver, &list) ? list : "DIRECT");
    }
    proxy_resolver_delete(&proxy_resolver);
}

static void execute_pac_script(const char *script_path, const char *url) {
    void *proxy_execute;

    printf("Executing PAC script %s for %s\n", script_path, url);

    // Open PAC script file
    int fd = open(script_path, O_RDONLY | O_RAW);
    if (fd < 0) {
        printf("Failed to open PAC script file %s\n", script_path);
        return;
    }

    // Get length of PAC script
    int32_t script_len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (script_len < 0) {
        printf("Failed to get length of PAC script file %s\n", script_path);
        goto execute_pac_cleanup;
    }

    // Allocate memory for PAC script
    char *script = (char *)calloc(script_len + 1, sizeof(char));
    if (!script) {
        printf("Failed to allocate memory for PAC script\n");
        goto execute_pac_cleanup;
    }

    // Read PAC script from file
    int32_t bytes_read = read(fd, script, script_len);
    if (bytes_read != script_len) {
        printf("Failed to read PAC script file %s\n", script_path);
        goto execute_pac_cleanup;
    }

    script[bytes_read] = 0;

    proxy_execute_create(&proxy_execute);
    if (proxy_execute_get_proxies_for_url(proxy_execute, script, url)) {
        char *list = proxy_execute_get_list(proxy_execute);
        printf("  Proxy: %s\n", list ? list : "DIRECT");
    }
    proxy_execute_delete(&proxy_execute);

execute_pac_cleanup:

    close(fd);
    free(script);
}

static int print_help(void) {
    printf("proxyres\n");
    printf("  config                  - dumps all proxy configuration values\n");
    printf("  execute [file] [urls..] - executes pac file with script\n");
    printf("  resolve [url..]         - resolves proxy for urls\n");
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc <= 1)
        return print_help();

    proxyres_init();

    const char *cmd = argv[1];
    if (strcmp(cmd, "config") == 0) {
        print_proxy_config();
    } else if (strcmp(cmd, "execute") == 0) {
        if (argc <= 3)
            return print_help();
        for (int i = 3; i < argc; i++)
            execute_pac_script(argv[2], argv[i]);
    } else if (strcmp(cmd, "resolve") == 0) {
        for (int i = 2; i < argc; i++)
            resolve_proxy_for_url(argv[i]);
    }
    proxyres_uninit();
    return 0;
}