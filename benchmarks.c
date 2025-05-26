#define _GNU_SOURCE     //for thread affinity
#include "benchmarks.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <unistd.h>

double get_time_diff(struct timespec start, struct timespec end)
{
    double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return elapsed_ns / 1e9;
}

// pin thread to a core
void set_thread_affinity(pthread_t thread, int cpu_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

void no_op_task(void *arg)
{
    volatile int dummy = 1;
    dummy = dummy + 1;
    (void)dummy; // prevent compiler optimization
}

double run_serial_spawn(threadpool_t *pool, int num_tasks)
{
    struct timespec start, end;

    printf("Running Serial Spawn with %d threads (%d tasks)...\n",
           pool->num_threads, num_tasks);

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_tasks; i++)
    {
        threadpool_submit(pool, no_op_task, NULL);
    }

    threadpool_wait_all(pool);

    clock_gettime(CLOCK_MONOTONIC, &end);

    return get_time_diff(start, end);
}

void parallel_spawner_task(void *arg)
{
    spawn_task_t *spawn_args = (spawn_task_t *)arg;
    threadpool_t *pool = spawn_args->pool;
    int tasks_to_spawn = spawn_args->tasks_to_spawn;

    for (int i = 0; i < tasks_to_spawn; i++)
    {
        threadpool_submit(pool, no_op_task, NULL);
    }
}

double run_parallel_spawn(threadpool_t *pool, int num_tasks)
{
    struct timespec start, end;

    printf("Running Parallel Spawn with %d threads (%d tasks total)\n",
           pool->num_threads, num_tasks);

    // each thread spawns tasks_per_thread tasks
    int tasks_per_thread = num_tasks / pool->num_threads;
    int remaining_tasks = num_tasks % pool->num_threads;

    spawn_task_t *spawn_args = malloc(sizeof(spawn_task_t) * pool->num_threads);
    if (!spawn_args)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1.0;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    // submit spawner tasks
    for (int i = 0; i < pool->num_threads; i++)
    {
        spawn_args[i].pool = pool;
        spawn_args[i].tasks_to_spawn = tasks_per_thread;
        if (i < remaining_tasks)
        {
            spawn_args[i].tasks_to_spawn++;
        }
        threadpool_submit(pool, parallel_spawner_task, &spawn_args[i]);
    }

    threadpool_wait_all(pool);

    clock_gettime(CLOCK_MONOTONIC, &end);

    free(spawn_args);
    return get_time_diff(start, end);
}

void fib_task(void *arg)
{
    fib_task_t *fib_args = (fib_task_t *)arg;

    if (fib_args->n < 2)
    {
        fib_args->value = fib_args->n;
        fib_args->completed = true;
        // signal completion
        pthread_mutex_lock(&fib_args->mutex);
        pthread_cond_signal(&fib_args->cond);
        pthread_mutex_unlock(&fib_args->mutex);
        return;
    }

 
    fib_task_t child1 = {
        .pool = fib_args->pool,
        .n = fib_args->n - 1,
        .value = 0,
        .remaining_children = 0,
        .completed = false
    };
    
    fib_task_t child2 = {
        .pool = fib_args->pool,
        .n = fib_args->n - 2,
        .value = 0,
        .remaining_children = 0,
        .completed = false
    };

    pthread_mutex_init(&child1.mutex, NULL);
    pthread_cond_init(&child1.cond, NULL);
    pthread_mutex_init(&child2.mutex, NULL);
    pthread_cond_init(&child2.cond, NULL);

    threadpool_submit(fib_args->pool, fib_task, &child1);
    threadpool_submit(fib_args->pool, fib_task, &child2);

    pthread_mutex_lock(&child1.mutex);
    while (!child1.completed)
    {
        pthread_cond_wait(&child1.cond, &child1.mutex);
    }
    pthread_mutex_unlock(&child1.mutex);

    pthread_mutex_lock(&child2.mutex);
    while (!child2.completed)
    {
        pthread_cond_wait(&child2.cond, &child2.mutex);
    }
    pthread_mutex_unlock(&child2.mutex);

    fib_args->value = child1.value + child2.value;
    fib_args->completed = true;

    pthread_mutex_lock(&fib_args->mutex);
    pthread_cond_signal(&fib_args->cond);
    pthread_mutex_unlock(&fib_args->mutex);

    pthread_mutex_destroy(&child1.mutex);
    pthread_cond_destroy(&child1.cond);
    pthread_mutex_destroy(&child2.mutex);
    pthread_cond_destroy(&child2.cond);
}

double run_fibonacci(threadpool_t *pool, int fib_number)
{
    struct timespec start, end;

    printf("Running Fibonacci(%d) with %d threads (recursive parallel)...\n", fib_number, pool->num_threads);

    fib_task_t fib_args = {
        .pool = pool,
        .n = fib_number,
        .value = 0,
        .remaining_children = 0,
        .completed = false};

    pthread_mutex_init(&fib_args.mutex, NULL);
    pthread_cond_init(&fib_args.cond, NULL);

    clock_gettime(CLOCK_MONOTONIC, &start);

    threadpool_submit(pool, fib_task, &fib_args);

    // Wait for the top-level task to complete
    pthread_mutex_lock(&fib_args.mutex);
    while (!fib_args.completed)
    {
        pthread_cond_wait(&fib_args.cond, &fib_args.mutex);
    }
    pthread_mutex_unlock(&fib_args.mutex);

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("Fibonacci(%d) = %ld\n", fib_number, fib_args.value);

    pthread_mutex_destroy(&fib_args.mutex);
    pthread_cond_destroy(&fib_args.cond);

    return get_time_diff(start, end);
}
