
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <windows.h>

#include "log.h"
#include "threadpool.h"

typedef struct threadpool_job_s {
    void *user_data;
    threadpool_job_cb callback;
    struct threadpool_job_s *next;
} threadpool_job_s;

struct threadpool_s;

typedef struct threadpool_thread_s {
    HANDLE handle;
    DWORD id;
    struct threadpool_s *pool;
    struct threadpool_thread_s *next;
} threadpool_thread_s;

typedef struct threadpool_s {
    bool stop;
    int32_t min_threads;
    int32_t num_threads;
    int32_t max_threads;
    int32_t busy_threads;
    HANDLE wakeup_cond;
    HANDLE lazy_cond;
    int32_t queue_count;
    CRITICAL_SECTION queue_lock;
    threadpool_job_s *queue_first;
    threadpool_job_s *queue_last;
    threadpool_thread_s *threads;
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

static bool threadpool_job_delete(threadpool_job_s **job) {
    if (job == NULL)
        return false;
    free(*job);
    *job = NULL;
    return true;
}

static void threadpool_enqueue_job(threadpool_s *threadpool, threadpool_job_s *job) {
    LOG_DEBUG("thread_pool - job 0x%" PRIxPTR " - enqueue\n", (intptr_t)job);

    // Add job to the end of the queue
    if (!threadpool->queue_last) {
        threadpool->queue_first = job;
        threadpool->queue_last = job;
    } else {
        threadpool->queue_last->next = job;
        threadpool->queue_last = job;
    }
    threadpool->queue_count++;
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

    LOG_DEBUG("thread_pool - job 0x%" PRIxPTR " - dequeue\n", (intptr_t)job);
    return job;
}

static DWORD WINAPI threadpool_do_work(LPVOID arg) {
    threadpool_thread_s *thread = (threadpool_thread_s *)arg;
    threadpool_s *threadpool = thread->pool;

    LOG_DEBUG("thread_pool - worker 0x%" PRIx32 " - started\n", thread->id);

    if (!thread)
        return 1;

    while (true) {
        EnterCriticalSection(&threadpool->queue_lock);
        LOG_DEBUG("thread_pool - worker 0x%" PRIx32 " - waiting for job\n", thread->id);

        // Sleep until there is work to do
        while (!threadpool->stop && !threadpool->queue_first) {
            LeaveCriticalSection(&threadpool->queue_lock);
            // queue_lock will be unlocked during sleep and locked during awake
            DWORD wait = WaitForSingleObject(threadpool->wakeup_cond, 250);
            EnterCriticalSection(&threadpool->queue_lock);
            if (wait == WAIT_TIMEOUT)
                continue;
        }

        if (threadpool->stop)
            break;

        // Get the next job to do
        threadpool_job_s *job = threadpool_dequeue_job(threadpool);

        // Increment count of busy threads
        threadpool->busy_threads++;
        LeaveCriticalSection(&threadpool->queue_lock);

        // Do the job
        if (job) {
            LOG_DEBUG("thread_pool - worker 0x%" PRIx32 " - processing job 0x%" PRIxPTR "\n", thread->id,
                      (intptr_t)job);
            job->callback(job->user_data);
            LOG_DEBUG("thread_pool - worker 0x%" PRIx32 " - job complete 0x%" PRIxPTR "\n", thread->id, (intptr_t)job);
            threadpool_job_delete(&job);
        }

        EnterCriticalSection(&threadpool->queue_lock);

        // Decrement count of busy threads
        threadpool->busy_threads--;

        // If no busy threads then signal threadpool_wait that we are lazy
        if (threadpool->busy_threads == 0 && !threadpool->queue_first)
            SetEvent(threadpool->lazy_cond);

        LeaveCriticalSection(&threadpool->queue_lock);
    }

    LOG_DEBUG("thread_pool - worker 0x%" PRIx32 " - stopped\n", thread->id);

    // Reduce thread count
    threadpool->num_threads--;
    SetEvent(threadpool->lazy_cond);
    LeaveCriticalSection(&threadpool->queue_lock);
    return 0;
}

static void threadpool_create_thread_on_demand(threadpool_s *threadpool) {
    // Create new thread and add it to the list of threads
    threadpool_thread_s *thread = (threadpool_thread_s *)calloc(1, sizeof(threadpool_thread_s));
    if (!thread)
        return;

    thread->pool = threadpool;
    thread->next = threadpool->threads;

    threadpool->threads = thread;
    threadpool->num_threads++;

    thread->handle = CreateThread(NULL, 0, threadpool_do_work, thread, 0, &thread->id);
}

bool threadpool_enqueue(void *ctx, void *user_data, threadpool_job_cb callback) {
    threadpool_s *threadpool = (threadpool_s *)ctx;

    // Create new job
    threadpool_job_s *job = threadpool_job_create(user_data, callback);
    if (!job)
        return false;

    EnterCriticalSection(&threadpool->queue_lock);

    // Add job to the job queue
    threadpool_enqueue_job(threadpool, job);

    // Create new thread if all threads are busy
    if (threadpool->busy_threads == threadpool->num_threads && threadpool->num_threads < threadpool->max_threads)
        threadpool_create_thread_on_demand(threadpool);

    LeaveCriticalSection(&threadpool->queue_lock);

    // Wake up waiting threads
    SetEvent(threadpool->wakeup_cond);
    return true;
}

static void threadpool_delete_threads(threadpool_s *threadpool) {
    threadpool_thread_s *thread = NULL;

    // Delete threads from list of threads
    while (threadpool->threads) {
        thread = threadpool->threads;
        threadpool->threads = threadpool->threads->next;
        threadpool->num_threads--;

        // Wait for thread to exit
        while (true) {
            // Signal wake up condition to wake up threads to stop
            SetEvent(threadpool->wakeup_cond);
            DWORD wait = WaitForSingleObject(thread->handle, 250);
            if (wait == WAIT_TIMEOUT)
                continue;
        }

        free(thread);
    }
}

static void threadpool_delete_jobs(threadpool_s *threadpool) {
    threadpool_job_s *job = NULL;

    // Delete job from the queue
    while (threadpool->queue_first) {
        job = threadpool->queue_first;
        threadpool->queue_first = threadpool->queue_first->next;
        threadpool_job_delete(&job);
    }
}

static void threadpool_stop_threads(threadpool_s *threadpool) {
    // Stop threads from doing anymore work
    threadpool->stop = true;

    // Wake up all threads to check stop flag
    SetEvent(threadpool->wakeup_cond);
}

void threadpool_wait(void *ctx) {
    threadpool_s *threadpool = (threadpool_s *)ctx;
    if (!threadpool)
        return;

    EnterCriticalSection(&threadpool->queue_lock);
    while (true) {
        if ((!threadpool->stop && (threadpool->busy_threads != 0 || threadpool->queue_count != 0)) ||
            (threadpool->stop && threadpool->num_threads != 0)) {
            LeaveCriticalSection(&threadpool->queue_lock);
            // Wait for signal that indicates there is no more work to do
            WaitForSingleObject(threadpool->lazy_cond, 250);
            EnterCriticalSection(&threadpool->queue_lock);
        } else {
            break;
        }
    }
    LeaveCriticalSection(&threadpool->queue_lock);
}

void *threadpool_create(int32_t min_threads, int32_t max_threads) {
    threadpool_s *threadpool = (threadpool_s *)calloc(1, sizeof(threadpool_s));
    if (!threadpool)
        return NULL;

    threadpool->min_threads = min_threads;
    threadpool->max_threads = max_threads;

    InitializeCriticalSection(&threadpool->queue_lock);

    threadpool->wakeup_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
    threadpool->lazy_cond = CreateEvent(NULL, FALSE, FALSE, NULL);

    return threadpool;
}

bool threadpool_delete(void **ctx) {
    threadpool_s *threadpool;
    if (!ctx)
        return false;
    threadpool = (threadpool_s *)*ctx;
    if (!threadpool)
        return false;

    threadpool_stop_threads(threadpool);
    threadpool_delete_threads(threadpool);
    threadpool_delete_jobs(threadpool);

    DeleteCriticalSection(&threadpool->queue_lock);

    if (threadpool->wakeup_cond)
        CloseHandle(threadpool->wakeup_cond);
    if (threadpool->lazy_cond)
        CloseHandle(threadpool->lazy_cond);

    free(threadpool);
    *ctx = NULL;
    return true;
}
