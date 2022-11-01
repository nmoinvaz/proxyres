#include <stdint.h>
#include <stdbool.h>

#include <windows.h>

#include "threadpool.h"

typedef struct threadpool_job_s {
    PTP_WORK handle;
    void *user_data;
    threadpool_job_cb callback;
    threadpool_s *pool;
    struct threadpool_job_s *next;
} threadpool_job_s;

typedef struct threadpool_s {
    PTP_POOL handle;
    TP_CALLBACK_ENVIRON cb_environ;

    CRITICAL_SECTION queue_lock;
    threadpool_job_s *queue_first;
    threadpool_job_s *queue_last;
} threadpool_s;

static threadpool_job_s *threadpool_job_create(void *user_data, threadpool_job_cb callback) {
    threadpool_job_s *job = (threadpool_job_s *)calloc(1, sizeof(threadpool_job_s));
    if (!job)
        return NULL;
    job->user_data = user_data;
    job->callback = callback;
    job->next = NULL;
    return job;
}

static bool threadpool_enqueue_job(threadpool_s *threadpool, threadpool_job_s *job) {
    LOG_DEBUG("thread_pool - job 0x%p - enqueue\n", job);

    // Add job to the end of the queue
    if (!threadpool->queue_last) {
        threadpool->queue_first = job;
        threadpool->queue_last = job;
    } else {
        threadpool->queue_last->next = job;
        threadpool->queue_last = job;
    }
    threadpool->queue_count++;
    return true;
}

static threadpool_job_s *threadpool_dequeue_job(threadpool_s *threadpool) {
    if (!threadpool->queue_first)
        return NULL;

    // Remove the first job from the queue
    threadpool_job_s *job = threadpool->queue_first;
    threadpool->queue_first = threadpool->queue_first->next;
    if (!threadpool->queue_first)
        threadpool->queue_last = NULL;
    threadpool->queue_count--;

    LOG_DEBUG("thread_pool - job 0x%p - dequeue\n", job);
    return job;
}

VOID CALLBACK threadpool_job_callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
    threadpool_job_s *job = (threadpool_job_s *)context;
    if (!job)
        return;

    DWORD thread_id = GetThreadId();

    // Execute the job
    LOG_DEBUG("thread_pool - worker 0x%u - processing job 0x%p\n", thread_id, job);
    job->callback(job->user_data);
    LOG_DEBUG("thread_pool - worker 0x%u - job complete 0x%p\n", thread_id, job);

    EnterCriticalSection(&job->pool->queue_lock);
    threadpool_dequeue_job(job);
    LeaveCriticalSection(&job->pool->queue_lock);
    // Free the job
    free(job);
}

bool threadpool_enqueue(void *ctx, void *user_data, threadpool_job_cb callback) {
    threadpool_s *threadpool = (threadpool_s *)ctx;

    threadpool_job_s *job = threadpool_job_create(user_data, callback);
    if (!job)
        return false;

    job->pool = threadpool;
    job->handle = CreateThreadpoolWork(threadpool_job_callback, job, &threadpool->cb_environ);
    if (!job->handle) {
        threadpool_job_delete(&job->handle);
        return false;
    }

    EnterCriticalSection(&threadpool->queue_lock);
    threadpool_enqueue_job(threadpool, job);
    LeaveCriticalSection(&threadpool->queue_lock);

    SubmitThreadpoolWork(job->handle);
    return true;
}
void threadpool_wait(void *ctx);

void *threadpool_create(int32_t min_threads, int32_t max_threads) {
    threadpool_s *threadpool = (threadpool_s *)calloc(1, sizeof(threadpool_s));
    TP_CALLBACK_ENVIRON CallBackEnviron;

    InitializeThreadpoolEnvironment(&threadpool->cb_environ);

    threadpool->handle = CreateThreadpool(NULL);
    if (!threadpool->handle) {
        free(threadpool);
        return NULL;
    }

    InitializeCriticalSection(&threadpool->queue_lock);

    SetThreadpoolThreadMinimum(threadpool->handle, min_threads);
    SetThreadpoolThreadMaximum(threadpool->handle, max_threads);

    SetThreadpoolCallbackPool(&threadpool->cb_environ, threadpool->pool);
    return true;
}

bool threadpool_delete(void **ctx) {
    if (!ctx || !*ctx)
        return false;
    threadpool_s *threadpool = (threadpool_s *)*ctx;
    if (threadpool->handle)
        CloseThreadpool(threadpool->handle);
    if (threadpool->queue_lock)
        DeleteCriticalSection(&threadpool->queue_lock);
    free(threadpool);
    *ctx = NULL;
    return true;
}