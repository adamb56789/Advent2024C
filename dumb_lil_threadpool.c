#include "dumb_lil_threadpool.h"

#include <assert.h>
#include <emmintrin.h>
#include <pthread.h>
#include <stdatomic.h>
#include <bits/pthreadtypes.h>

#define MAX_THREADS 10

typedef struct {
    pthread_t pthread;
    atomic_int keepAlive;
    atomic_int workAvailable;
    void *arg;

    void (*function)(void *);
} PoolThread;

static _Atomic int threadsRunning_atomic = 0;
static PoolThread threads[MAX_THREADS];

static void *worker(void *arg) {
    PoolThread *thread = arg;

    while (1) {
        while (!thread->workAvailable) _mm_pause();

        // Threads die when they are killed
        if (!thread->keepAlive) return NULL;

        thread->workAvailable = 0;

        thread->function(thread->arg);

        threadsRunning_atomic--;
    }
}

void dumb_lil_threadpool_init(const int numThreads) {
    assert(numThreads <= MAX_THREADS);
    for (int i = 0; i < numThreads; i++) {
        threads[i].keepAlive = 1;
        threads[i].workAvailable = 0;
        threads[i].arg = NULL;
        threads[i].function = NULL;
        pthread_create(&threads[i].pthread, NULL, worker, &threads[i]);
    }
}

void dumb_lil_threadpool_add_work(void (*function)(void *), void *arg, const int threadId) {
    threadsRunning_atomic++;
    threads[threadId].function = function;
    threads[threadId].arg = arg;
    threads[threadId].workAvailable = 1;
}

void dumb_lil_threadpool_wait() {
    while (threadsRunning_atomic != 0) _mm_pause();
}

void dumb_lil_threadpool_destroy() {
    for (int i = 0; i < MAX_THREADS; i++) {
        // Mark as unalive, then wake it up so it can kill itself
        threads[i].keepAlive = 0;
        threads[i].workAvailable = 1;
        pthread_join(threads[i].pthread, NULL);
    }
}
