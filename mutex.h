#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Lock mutex.
bool mutex_lock(void *ctx);

// Unlock mutex that has been previously locked.
bool mutex_unlock(void *ctx);

// Creates a mutex instance.
void *mutex_create(void);

// Deletes a mutex instance.
bool mutex_delete(void **ctx);

#ifdef __cplusplus
}
#endif
