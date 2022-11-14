#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <pthread.h>

#include "log.h"
#include "threadpool.h"

typedef struct threadpool_job_s {
    void *user_data;
    threadpool_job_cb callback;
    struct threadpool_job_s *next;
} threadpool_job_s;

struct threadpool_s;

typedef struct threadpool_thread_s {
    pthread_t handle;
    struct threadpool_s *pool;
    struct threadpool_thread_s *next;
} threadpool_thread_s;

typedef struct threadpool_s {
    bool stop;
    int32_t min_threads;
    int32_t num_threads;
    int32_t max_threads;
    int32_t busy_threads;
    pthread_cond_t wakeup_cond;
    pthread_cond_t lazy_cond;
    int32_t queue_count;
    pthread_mutex_t queue_mutex;
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

static bool threadpool_enqueue_job(threadpool_s *threadpool, threadpool_job_s *job) {
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

    LOG_DEBUG("thread_pool - job 0x%" PRIxPTR " - dequeue\n", (intptr_t)job);
    return job;
}

static void *threadpool_do_work(void *arg) {
    threadpool_thread_s *thread = (threadpool_thread_s *)arg;
    threadpool_s *threadpool = thread->pool;

    LOG_DEBUG("thread_pool - worker 0x%" PRIx64 " - started\n", (uint64_t)pthread_self());

    if (!thread)
        return NULL;

    while (true) {
        pthread_mutex_lock(&threadpool->queue_mutex);
        LOG_DEBUG("thread_pool - worker 0x%" PRIx64 " - waiting for job\n", (uint64_t)pthread_self());

        // Sleep until there is work to do
        while (!threadpool->stop && !threadpool->queue_first) {
            // Queue_mutex will be unlocked during sleep and locked during awake
            pthread_cond_wait(&threadpool->wakeup_cond, &threadpool->queue_mutex);
        }

        if (threadpool->stop)
            break;

        // Get the next job to do
        threadpool_job_s *job = threadpool_dequeue_job(threadpool);

        // Increment count of busy threads
        threadpool->busy_threads++;
        pthread_mutex_unlock(&threadpool->queue_mutex);

        // Do the job
        if (job) {
            LOG_DEBUG("thread_pool - worker 0x%" PRIx64 " - processing job 0x%" PRIxPTR "\n", (uint64_t)pthread_self(), (intptr_t)job);
            job->callback(job->user_data);
            LOG_DEBUG("thread_pool - worker 0x%" PRIx64 " - job complete 0x%" PRIxPTR "\n", (uint64_t)pthread_self(), (intptr_t)job);
            threadpool_job_delete(&job);
        }

        pthread_mutex_lock(&threadpool->queue_mutex);

        // Decrement count of busy threads
        threadpool->busy_threads--;

        // If no busy threads then signal threadpool_wait that we are lazy
        if (threadpool->busy_threads == 0 && !threadpool->queue_first)
            pthread_cond_signal(&threadpool->lazy_cond);

        pthread_mutex_unlock(&threadpool->queue_mutex);
    }

    LOG_DEBUG("thread_pool - worker 0x%" PRIx64 " - stopped\n", (uint64_t)pthread_self());

    // Reduce thread count
    threadpool->num_threads--;
    pthread_cond_signal(&threadpool->lazy_cond);
    pthread_mutex_unlock(&threadpool->queue_mutex);
    return NULL;
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

    pthread_create(&thread->handle, NULL, threadpool_do_work, thread);
}

bool threadpool_enqueue(void *ctx, void *user_data, threadpool_job_cb callback) {
    threadpool_s *threadpool = (threadpool_s *)ctx;

    // Create new job
    threadpool_job_s *job = threadpool_job_create(user_data, callback);
    if (!job)
        return false;

    pthread_mutex_lock(&threadpool->queue_mutex);

    // Add job to the job queue
    threadpool_enqueue_job(threadpool, job);

    // Create new thread if all threads are busy
    if (threadpool->busy_threads == threadpool->num_threads && threadpool->num_threads < threadpool->max_threads)
        threadpool_create_thread_on_demand(threadpool);

    pthread_mutex_unlock(&threadpool->queue_mutex);

    // Wake up waiting threads
    pthread_cond_broadcast(&threadpool->wakeup_cond);
    return true;
}

static void threadpool_delete_threads(threadpool_s *threadpool) {
    threadpool_thread_s *thread = NULL;

    // Delete threads from list of threads
    while (threadpool->threads) {
        thread = threadpool->threads;
        threadpool->threads = threadpool->threads->next;
        threadpool->num_threads--;

        pthread_join(thread->handle, NULL);

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
    pthread_cond_broadcast(&threadpool->wakeup_cond);
}

void threadpool_wait(void *ctx) {
    threadpool_s *threadpool = (threadpool_s *)ctx;
    if (!threadpool)
        return;

    pthread_mutex_lock(&threadpool->queue_mutex);
    while (true) {
        if ((!threadpool->stop && (threadpool->busy_threads != 0 || threadpool->queue_count != 0)) ||
            (threadpool->stop && threadpool->num_threads != 0)) {
            // Wait for signal that indicates there is no more work to do
            pthread_cond_wait(&threadpool->lazy_cond, &threadpool->queue_mutex);
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&threadpool->queue_mutex);
}

void *threadpool_create(int32_t min_threads, int32_t max_threads) {
    threadpool_s *threadpool = (threadpool_s *)calloc(1, sizeof(threadpool_s));
    if (!threadpool)
        return NULL;

    threadpool->min_threads = min_threads;
    threadpool->max_threads = max_threads;

    pthread_mutex_init(&threadpool->queue_mutex, NULL);
    pthread_cond_init(&threadpool->wakeup_cond, NULL);
    pthread_cond_init(&threadpool->lazy_cond, NULL);

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

    pthread_mutex_destroy(&threadpool->queue_mutex);
    pthread_cond_destroy(&threadpool->wakeup_cond);
    pthread_cond_destroy(&threadpool->lazy_cond);

    free(threadpool);
    *ctx = NULL;
    return true;
}
