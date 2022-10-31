#pragma once

#define THREADPOOL_DEFAULT_MIN_THREADS 1
#define THREADPOOL_DEFAULT_MAX_THREADS 3

typedef void (*threadpool_job_cb)(void *user_data);

bool threadpool_enqueue(void *ctx, void *user_data, threadpool_job_cb callback);
void threadpool_wait(void *ctx);

void *threadpool_create(int32_t min_threads, int32_t max_threads);
bool threadpool_delete(void **ctx);