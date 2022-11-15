#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <windows.h>

typedef struct signal_s {
    HANDLE handle;
} signal_s;

bool signal_wait(void *ctx, int32_t timeout_ms) {
    signal_s *signal = (signal_s *)ctx;
    if (!signal)
        return false;
    if (WaitForSingleObject(signal->handle, timeout_ms ? timeout_ms : INFINITE) != WAIT_OBJECT_0)
        return false;
    return true;
}

bool signal_reset(void *ctx) {
    signal_s *signal = (signal_s *)ctx;
    if (!signal)
        return false;
    if (!ResetEvent(signal->handle))
        return false;
    return true;
}

bool signal_set(void *ctx) {
    signal_s *signal = (signal_s *)ctx;
    if (!signal || !SetEvent(signal->handle))
        return false;
    return true;
}

void *signal_create(void) {
    signal_s *signal = (signal_s *)calloc(1, sizeof(signal_s));
    if (!signal)
        return NULL;
    signal->handle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!signal->handle) {
        free(signal);
        return NULL;
    }
    return signal;
}

bool signal_delete(void **ctx) {
    if (!ctx)
        return false;
    signal_s *signal = (signal_s *)*ctx;
    if (!signal)
        return false;
    CloseHandle(signal->handle);
    free(signal);
    *ctx = NULL;
    return true;
}
