#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "proxyres.h"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define O_BINARY _O_BINARY
#define usleep Sleep
#else
#include <unistd.h>
#define O_BINARY 0
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

#if 0
static void resolve_proxy_for_url(const char *url) {
    void *proxy_resolver = proxy_resolver_create();
    if (!proxy_resolver)
        return;

    printf("Resolving proxy for %s\n", url);

    if (proxy_resolver_get_proxies_for_url(proxy_resolver, url)) {
        // Wait for proxy to resolve asynchronously
        while (proxy_resolver_is_pending(proxy_resolver)) {
            usleep(10);
        }

        // Get the proxy list for the url
        const char *list = proxy_resolver_get_list(proxy_resolver);
        printf("  Proxy: %s\n", list ? list : "DIRECT");
    }
    proxy_resolver_delete(&proxy_resolver);
}
#endif

static bool resolve_proxy_for_url_async(int argc, char *argv[]) {
    bool success = false;
    int32_t error = 0;
    
    void **proxy_resolver = (void **)calloc(argc, sizeof(void *));
    if (!proxy_resolver)
        return false;

    for (int32_t i = 0; i < argc; i++) {
        // Create each proxy resolver instance
        proxy_resolver[i] = proxy_resolver_create();
        if (!proxy_resolver[i])
            return false;

        // Start asynchronous request for proxy resolution
        proxy_resolver_get_proxies_for_url(proxy_resolver[i], argv[i]);
    }

    success = true;
    for (int32_t i = 0; i < argc; i++) {
        printf("Resolving proxy for %s\n", argv[i]);

        // Wait for proxy to resolve asynchronously
        while (proxy_resolver_is_pending(proxy_resolver[i])) {
            usleep(10);
        }

        // Get the proxy list for the url
        const char *list = proxy_resolver_get_list(proxy_resolver[i]);
        printf("  Proxy: %s\n", list ? list : "DIRECT");

        proxy_resolver_get_error(proxy_resolver[i], &error);
        if (error != 0)
            success = false;

        proxy_resolver_delete(&proxy_resolver[i]);
    }
    
    return success;
}

static bool execute_pac_script(const char *script_path, const char *url) {
    bool success = false;
    char *script = NULL;
    
    printf("Executing PAC script %s for %s\n", script_path, url);

    // Open PAC script file
    int fd = open(script_path, O_RDONLY | O_BINARY);
    if (fd < 0) {
        printf("Failed to open PAC script file %s\n", script_path);
        goto execute_pac_cleanup;
    }

    // Get length of PAC script
    int32_t script_len = lseek(fd, 0, SEEK_END);
    if (script_len < 0) {
        printf("Failed to get length of PAC script file %s\n", script_path);
        goto execute_pac_cleanup;
    }

    // Allocate memory for PAC script
    script = (char *)calloc(script_len + 1, sizeof(char));
    if (!script) {
        printf("Failed to allocate memory for PAC script\n");
        goto execute_pac_cleanup;
    }

    // Read PAC script from file
    lseek(fd, 0, SEEK_SET);
    int32_t bytes_read = read(fd, script, script_len);
    if (bytes_read != script_len) {
        printf("Failed to read PAC script file %s (%d != %d)\n", script_path, bytes_read, script_len);
        goto execute_pac_cleanup;
    }

    script[bytes_read] = 0;

    void *proxy_execute = proxy_execute_create();
    if (proxy_execute) {
        if (proxy_execute_get_proxies_for_url(proxy_execute, script, url)) {
            const char *list = proxy_execute_get_list(proxy_execute);
            printf("  Proxy: %s\n", list ? list : "DIRECT");
        }
        proxy_execute_delete(&proxy_execute);
    }

    success = true;

execute_pac_cleanup:

    close(fd);
    free(script);

    return success;
}

static int print_help(void) {
    printf("proxyres\n");
    printf("  config                  - dumps all proxy configuration values\n");
    printf("  execute [file] [urls..] - executes pac file with script\n");
    printf("  resolve [url..]         - resolves proxy for urls\n");
    return 1;
}

int main(int argc, char *argv[]) {
    int exit_code = 0;

    if (argc <= 1)
        return print_help();

    proxyres_init();

    const char *cmd = argv[1];
    if (strcmp(cmd, "config") == 0) {
        print_proxy_config();
    } else if (strcmp(cmd, "execute") == 0) {
        if (argc <= 3)
            return print_help();

        for (int32_t i = 3; i < argc; i++) {
            if (!execute_pac_script(argv[2], argv[i]))
                exit_code = 1;
        }
    } else if (strcmp(cmd, "resolve") == 0) {
        if (!resolve_proxy_for_url_async(argc - 2, argv + 2))
            exit_code = 1;
    }

    proxyres_uninit();
    return exit_code;
}
