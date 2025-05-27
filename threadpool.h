#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

extern __thread bool is_worker;

/*
    To remove overhead of thread allocation threads are preallocated in a pool
*/

typedef struct {
    void (*func)(void *arg);
    void *arg;
} task_t;


typedef struct {
    pthread_t *workers;
    int num_threads;
    
    task_t *queue;
    int queue_size;
    int queue_capacity;
    int queue_front;
    int queue_rear;
    
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;
    
    atomic_int outstanding_tasks;
    pthread_mutex_t done_mutex;
    pthread_cond_t done_cond;
    
    bool shutdown_flag;
} threadpool_t;


threadpool_t* threadpool_create(int num_threads);
void threadpool_submit(threadpool_t *pool, void (*func)(void*), void *arg);
void threadpool_wait_all(threadpool_t *pool);
void threadpool_destroy(threadpool_t *pool);

void* worker_thread(void *arg);

#endif // THREADPOOL_H
