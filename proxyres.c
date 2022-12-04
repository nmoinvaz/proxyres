#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#ifdef PROXYRES_EXECUTE
#  include "execute.h"
#endif
#include "log.h"
#include "resolver.h"

bool proxyres_global_init(void) {
    if (!proxy_config_global_init())
        LOG_WARN("Failed to initialization proxy config\n");
#ifdef PROXYRES_EXECUTE
    if (!proxy_execute_global_init())
        LOG_WARN("Failed to initialization proxy execute\n");
#endif
    if (!proxy_resolver_global_init())
        LOG_WARN("Failed to initialization proxy resolver\n");
    return true;
}

bool proxyres_global_cleanup(void) {
    proxy_resolver_global_cleanup();
#ifdef PROXYRES_EXECUTE
    proxy_execute_global_cleanup();
#endif
    proxy_config_global_cleanup();
    return true;
}
