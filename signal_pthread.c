#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <pthread.h>

typedef struct signal_s {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} signal_s;

bool signal_set(void *ctx) {
    signal_s *signal = (signal_s *)ctx;
    if (!signal)
        return false;
    pthread_mutex_lock(&signal->mutex);
    int32_t err = pthread_cond_signal(&signal->cond);
    pthread_mutex_unlock(&signal->mutex);
    return err == 0;
}

bool signal_wait(void *ctx, int32_t timeout_ms) {
    signal_s *signal = (signal_s *)ctx;
    int32_t err = 0;

    if (!signal)
        return false;

    pthread_mutex_lock(&signal->mutex);
    if (timeout_ms < 0) {
        err = pthread_cond_wait(&signal->cond, &signal->mutex);
    } else {
        struct timespec ts;

        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;

        err = pthread_cond_timedwait(&signal->cond, &signal->mutex, &ts);
    }
    pthread_mutex_unlock(&signal->mutex);
    return err == 0;
}

void *signal_create(void) {
    signal_s *signal = (signal_s *)calloc(1, sizeof(signal_s));
    if (!signal)
        return NULL;
    if (pthread_cond_init(&signal->cond, NULL)) {
        free(signal);
        return NULL;
    }
    if (pthread_mutex_init(&signal->mutex, NULL)) {
        pthread_cond_destroy(&signal->cond);
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
    pthread_cond_destroy(&signal->cond);
    pthread_mutex_destroy(&signal->mutex);
    free(signal);
    *ctx = NULL;
    return true;
}
