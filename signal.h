#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool signal_set(void *ctx);
bool signal_reset(void *ctx);
bool signal_is_set(void *ctx);
bool signal_wait(void *ctx, int32_t timeout_ms);

void *signal_create(void);
bool signal_delete(void **ctx);

#ifdef __cplusplus
}
#endif
