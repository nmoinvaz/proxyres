#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool event_set(void *ctx);
bool event_is_set(void *ctx);
bool event_wait(void *ctx, int32_t timeout_ms);

void *event_create(void);
bool event_delete(void **ctx);

#ifdef __cplusplus
}
#endif
