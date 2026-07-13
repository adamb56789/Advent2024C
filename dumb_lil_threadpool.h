#ifndef ADVENT2024C_DUMB_LIL_THREADPOOL_H
#define ADVENT2024C_DUMB_LIL_THREADPOOL_H


void dumb_lil_threadpool_init(int numThreads);

void dumb_lil_threadpool_add_work(void (*function)(void *), void *arg, int threadId);

void dumb_lil_threadpool_wait();

void dumb_lil_threadpool_destroy();


#endif //ADVENT2024C_DUMB_LIL_THREADPOOL_H
