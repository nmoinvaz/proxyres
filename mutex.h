#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool mutex_lock(void *ctx);
bool mutex_unlock(void *ctx);

void *mutex_create(void);
bool mutex_delete(void **ctx);

#ifdef __cplusplus
}
#endif
