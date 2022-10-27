#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "resolver.h"

#ifdef __cplusplus
extern "C" {
#endif

bool proxyres_init(void);
bool proxyres_uninit(void);

#ifdef __cplusplus
}
#endif
