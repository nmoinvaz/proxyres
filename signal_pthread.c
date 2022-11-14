#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <pthread.h>

typedef struct signal_s {
    pthread_cond_t cond;
} signal_s;

bool signal_set(void *ctx) {
    signal_s *signal = (signal_s *)ctx;
    if (!signal || pthread_cond_signal(&signal->cond))
        return false;
    return true;
}

bool signal_wait(void *ctx, int32_t timeout_ms) {
    signal_s *signal = (signal_s *)ctx;
    if (!signal)
        return false;
    if (timeout_ms < 0) {
        if (pthread_cond_wait(&signal->cond, NULL))
            return false;
    } else {
        struct timespec ts;

        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;

        if (pthread_cond_timedwait(&signal->cond, NULL, &ts))
            return false;
    }
    return true;
}

void *signal_create(void) {
    signal_s *signal = (signal_s *)calloc(1, sizeof(signal_s));
    if (!signal)
        return NULL;
    if (pthread_cond_init(&signal->cond, NULL)) {
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
    if (pthread_cond_destroy(&signal->cond)) {
        free(signal);
        *ctx = NULL;
        return false;
    }
    free(signal);
    *ctx = NULL;
    return true;
}
