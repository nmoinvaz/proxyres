#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#ifdef PROXYRES_EXECUTE
#  include "execute.h"
#endif
#include "log.h"
#include "resolver.h"

bool proxyres_init(void) {
    if (!proxy_config_init())
        LOG_WARN("Failed to initialization proxy config\n");
#ifdef PROXYRES_EXECUTE
    if (!proxy_execute_init())
        LOG_WARN("Failed to initialization proxy execute\n");
#endif
    if (!proxy_resolver_init())
        LOG_WARN("Failed to initialization proxy resolver\n");
    return true;
}

bool proxyres_uninit(void) {
    proxy_resolver_uninit();
#ifdef PROXYRES_EXECUTE
    proxy_execute_uninit();
#endif
    proxy_config_uninit();
    return true;
}
