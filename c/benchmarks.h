#ifndef BENCHMARKS_H
#define BENCHMARKS_H

#include "threadpool.h"
#include <stdbool.h>

typedef enum
{
    BENCHMARK_SERIAL,
    BENCHMARK_PARALLEL,
    BENCHMARK_FIBONACCI
} benchmark_type_t;

typedef struct
{
    benchmark_type_t type;
    int num_threads;
    int num_tasks;
    int fib_number;
    bool pin_threads;
} benchmark_config_t;

typedef struct
{
    threadpool_t *pool;
    int tasks_to_spawn;
} spawn_task_t;

typedef struct fib_ctx {
    int n;
    long res;
    atomic_int pending;
    struct fib_ctx *parent;
    threadpool_t *pool;
} fib_ctx;

double run_serial_spawn(threadpool_t *pool, int num_tasks, bool simple);
double run_parallel_spawn(threadpool_t *pool, int num_tasks);
double run_fibonacci(threadpool_t *pool, int fib_number);

void no_op_task(void *arg);
void parallel_spawner_task(void *arg);
void fibonacci_task(void *arg);
void fibonacci_finish_task(void *arg);

void set_thread_affinity(pthread_t thread, int cpu_id);
double get_time_diff(struct timespec start, struct timespec end);

#endif // BENCHMARKS_H
