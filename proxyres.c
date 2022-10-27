#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "resolver.h"

bool proxyres_init(void) {
    return proxy_config_init() && proxy_resolver_init();
}

bool proxyres_uninit(void) {
    return proxy_config_uninit() && proxy_resolver_uninit();
}
