#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Add this line
__thread bool is_worker = false;

#define DEFAULT_QUEUE_CAPACITY 700000 // Increased from 10000

threadpool_t *threadpool_create(int num_threads)
{
    threadpool_t *pool = malloc(sizeof(threadpool_t));
    if (!pool)
        return NULL;

    pool->num_threads = num_threads;
    pool->workers = malloc(sizeof(pthread_t) * num_threads);
    if (!pool->workers)
    {
        free(pool);
        return NULL;
    }

    pool->queue_capacity = DEFAULT_QUEUE_CAPACITY;
    pool->queue = malloc(sizeof(task_t) * pool->queue_capacity);
    if (!pool->queue)
    {
        free(pool->workers);
        free(pool);
        return NULL;
    }
    pool->queue_size = 0;
    pool->queue_front = 0;
    pool->queue_rear = 0;

    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0 ||
        pthread_cond_init(&pool->queue_not_empty, NULL) != 0 ||
        pthread_cond_init(&pool->queue_not_full, NULL) != 0 ||
        pthread_mutex_init(&pool->done_mutex, NULL) != 0 ||
        pthread_cond_init(&pool->done_cond, NULL) != 0)
    {

        free(pool->queue);
        free(pool->workers);
        free(pool);
        return NULL;
    }


    atomic_init(&pool->outstanding_tasks, 0);
    pool->shutdown_flag = false;

    // Create worker threads
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_create(&pool->workers[i], NULL, worker_thread, pool) != 0)
        {
            pool->shutdown_flag = true;
            pthread_cond_broadcast(&pool->queue_not_empty);
            for (int j = 0; j < i; j++)
            {
                pthread_join(pool->workers[j], NULL);
            }
            pthread_mutex_destroy(&pool->queue_mutex);
            pthread_cond_destroy(&pool->queue_not_empty);
            pthread_cond_destroy(&pool->queue_not_full);
            pthread_mutex_destroy(&pool->done_mutex);
            pthread_cond_destroy(&pool->done_cond);
            free(pool->queue);
            free(pool->workers);
            free(pool);
            return NULL;
        }
    }

    return pool;
}


void *worker_thread(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    is_worker = true; // Set is_worker to true

    while (true)
    {
        pthread_mutex_lock(&pool->queue_mutex);

        while (pool->queue_size == 0 && !pool->shutdown_flag)
        {
            pthread_cond_wait(&pool->queue_not_empty, &pool->queue_mutex);
        }

        if (pool->shutdown_flag && pool->queue_size == 0)
        {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }

        // next task
        task_t task = pool->queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->queue_capacity;
        pool->queue_size--;

        pthread_cond_signal(&pool->queue_not_full);

        pthread_mutex_unlock(&pool->queue_mutex);

        task.func(task.arg);

        int remaining = atomic_fetch_sub(&pool->outstanding_tasks, 1) - 1;
        if (remaining == 0)
        {
            pthread_mutex_lock(&pool->done_mutex);
            pthread_cond_signal(&pool->done_cond);
            pthread_mutex_unlock(&pool->done_mutex);
        }
    }

    pthread_exit(NULL);
}

// new task to the pool
void threadpool_submit(threadpool_t *pool, void (*func)(void *), void *arg)
{
    // D-4: Inline-fallback on full queue
    if (is_worker && pool->queue_size == pool->queue_capacity) {
        func(arg);
        return;
    }

    atomic_fetch_add(&pool->outstanding_tasks, 1);

    pthread_mutex_lock(&pool->queue_mutex);

    while (pool->queue_size == pool->queue_capacity && !pool->shutdown_flag)
    {
        pthread_cond_wait(&pool->queue_not_full, &pool->queue_mutex);
    }

    if (pool->shutdown_flag)
    {
        pthread_mutex_unlock(&pool->queue_mutex);
        atomic_fetch_sub(&pool->outstanding_tasks, 1);
        return;
    }

    task_t task = {func, arg};
    pool->queue[pool->queue_rear] = task;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_capacity;
    pool->queue_size++;

    pthread_cond_signal(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);
}

void threadpool_wait_all(threadpool_t *pool)
{
    pthread_mutex_lock(&pool->done_mutex);
    while (atomic_load(&pool->outstanding_tasks) > 0)
    {
        pthread_cond_wait(&pool->done_cond, &pool->done_mutex);
    }
    pthread_mutex_unlock(&pool->done_mutex);
}

void threadpool_destroy(threadpool_t *pool)
{
    if (!pool)
        return;

    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown_flag = true;
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);

    // Join all worker threads
    for (int i = 0; i < pool->num_threads; i++)
    {
        pthread_join(pool->workers[i], NULL);
    }

    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_not_empty);
    pthread_cond_destroy(&pool->queue_not_full);
    pthread_mutex_destroy(&pool->done_mutex);
    pthread_cond_destroy(&pool->done_cond);

    free(pool->queue);
    free(pool->workers);
    free(pool);
}
