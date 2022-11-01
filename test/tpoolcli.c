#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "threadpool_pthread.h"

static const size_t num_items = 100;

void worker(void *arg)
{
    int *val = arg;
    int  old = *val;

    *val += 1000;
    printf("tid=%p, old=%d, val=%d\n", pthread_self(), old, *val);

    if (*val%2)
        usleep(100000);
}

int main(int argc, char **argv)
{
    int     *vals;
    size_t   i;

    void *tm   = threadpool_create(1, 10);
    vals = calloc(num_items, sizeof(*vals));

    for (i=0; i<num_items; i++) {
        vals[i] = i;
        threadpool_queue(tm, vals+i, worker);
    }

    threadpool_wait(tm);

    for (i=0; i<num_items; i++) {
        printf("%d\n", vals[i]);
    }

    free(vals);
    threadpool_delete(&tm);
    return 0;
}