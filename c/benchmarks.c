#define _GNU_SOURCE     //for thread affinity
#include "benchmarks.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <unistd.h>
#include <stdatomic.h>
#include <assert.h>

#define SERIAL_CUTOFF

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

int fib_rec(int n) {
    if (n < 2) return n;
    else
        return fib_rec(n - 1) + fib_rec(n - 2);
}

void no_op_task(void *arg)
{
    volatile int dummy = fib_rec(20);
    (void)dummy; // prevent compiler optimization
}

double run_serial_spawn(threadpool_t *pool, int num_tasks, bool simple)
{
    struct timespec start, end;

    /*printf("Running Serial Spawn with %d threads (%d tasks)...\n",
            pool->num_threads, num_tasks);*/

    pthread_t thread;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (!simple) {
        for (int i = 0; i < num_tasks; i++)
        {
            int ret = pthread_create(&thread, NULL, &no_op_task, NULL);

            if(ret != 0) {
                printf ("Create pthread error!\n");
                exit (1);
            }

            pthread_join(thread, NULL);
        }
        threadpool_wait_all(pool);
    } else {
        for (int i = 0; i < num_tasks; i++)
        {
            no_op_task(NULL);
        }
    }

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

    /*printf("Running Parallel Spawn with %d threads (%d tasks total)\n",
            pool->num_threads, num_tasks);*/

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
// Fixed version of the parallel fibonacci implementation

#ifdef SERIAL_CUTOFF
static long fib_serial(int n) {
    if (n < 2) return n;
    long a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        long temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}
#endif

void fibonacci_finish_task(void *arg)
{
    fib_ctx *ctx = (fib_ctx *)arg;

    if (ctx->parent) {
        // FIX 1: Use atomic addition to prevent race conditions
        atomic_fetch_add(&ctx->parent->res, ctx->res);
        
        // FIX 2: Check if this was the last child to complete
        if (atomic_fetch_sub(&ctx->parent->pending, 1) == 1) {
            threadpool_submit(ctx->pool, fibonacci_finish_task, ctx->parent);
        }
        free(ctx);
    }
    // Root context cleanup is handled by run_fibonacci
}

void fibonacci_task(void *arg)
{
    fib_ctx *ctx = (fib_ctx *)arg;

    if (ctx->n < 2) {
        ctx->res = ctx->n;
        threadpool_submit(ctx->pool, fibonacci_finish_task, ctx);
        return;
    }

#ifdef SERIAL_CUTOFF
    // FIX 3: Increase cutoff for better performance
    if (ctx->n <= 20) {  // Increased from 10 to 20
        ctx->res = fib_serial(ctx->n);
        threadpool_submit(ctx->pool, fibonacci_finish_task, ctx);
        return;
    }
#endif

    fib_ctx *left = malloc(sizeof(fib_ctx));
    fib_ctx *right = malloc(sizeof(fib_ctx));

    if (!left || !right) {
        if (left) free(left);
        if (right) free(right);
        ctx->res = -1;
        threadpool_submit(ctx->pool, fibonacci_finish_task, ctx);
        return;
    }

    *left = (fib_ctx){
        .n = ctx->n - 1,
        .res = 0,
        .pending = ATOMIC_VAR_INIT(0),
        .parent = ctx,
        .pool = ctx->pool
    };
    
    *right = (fib_ctx){
        .n = ctx->n - 2,
        .res = 0,
        .pending = ATOMIC_VAR_INIT(0),
        .parent = ctx,
        .pool = ctx->pool
    };

    atomic_store(&ctx->pending, 2);

    // FIX 4: Submit both children to thread pool for better load balancing
    threadpool_submit(ctx->pool, fibonacci_task, left);
    threadpool_submit(ctx->pool, fibonacci_task, right);
}

double run_fibonacci(threadpool_t *pool, int fib_number)
{
    struct timespec start, end;

    fib_ctx *root = calloc(1, sizeof(fib_ctx));
    if (!root) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1.0;
    }

    *root = (fib_ctx){
        .n = fib_number,
        .res = 0,  // FIX 5: Make sure res is atomic in struct definition
        .pending = ATOMIC_VAR_INIT(0),
        .parent = NULL,
        .pool = pool
    };

    clock_gettime(CLOCK_MONOTONIC, &start);

    threadpool_submit(pool, fibonacci_task, root);
    threadpool_wait_all(pool);

    clock_gettime(CLOCK_MONOTONIC, &end);

    free(root);

    return get_time_diff(start, end);
}
