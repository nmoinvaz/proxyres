#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#include "execute.h"
#include "log.h"
#include "resolver.h"

bool proxyres_init(void) {
    if (!proxy_config_init())
        LOG_WARN("Failed to initialization proxy config\n");
    if (!proxy_resolver_init())
        LOG_WARN("Failed to initialization proxy resolver\n");
    if (!proxy_execute_init())
        LOG_WARN("Failed to initialization proxy execute\n");
    return true;
}

bool proxyres_uninit(void) {
    proxy_config_uninit();
    proxy_resolver_uninit();
    proxy_execute_uninit();
    return true;
}
