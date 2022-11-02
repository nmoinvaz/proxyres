#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "log.h"
#include "threadpool.h"

#ifdef __linux__
#include <pthread.h>
#include <unistd.h>
#define worker_id ((uint32_t)pthread_self())
#else
#include <windows.h>
#define worker_id ((uint32_t)GetThreadId(NULL))
#define usleep    Sleep
#endif

#define NUM_ITEMS 100

static void worker(void *arg) {
    int32_t *val = arg;
    int32_t old = *val;

    *val += 1000;
    LOG_INFO("tid=%" PRIx32 ", old=%" PRId32 ", val=%" PRId32 "\n", worker_id, old, *val);

    if (*val % 2)
        usleep(1000);
}

int main(int argc, char **argv) {
    void *threadpool = threadpool_create(1, 10);
    if (!threadpool)
        return 1;
    
    int32_t *vals = (int32_t *)calloc(NUM_ITEMS, sizeof(*vals));
    if (!vals) {
        threadpool_delete(&threadpool);
        return 1;
    }

    for (size_t i = 0; i < NUM_ITEMS; i++) {
        vals[i] = (int32_t)i;
        threadpool_enqueue(threadpool, vals + i, worker);
    }

    threadpool_wait(threadpool);

    for (size_t i = 0; i < NUM_ITEMS; i++)
        LOG_DEBUG("%" PRId32 "\n", vals[i]);

    free(vals);
    threadpool_delete(&threadpool);
    return 0;
}