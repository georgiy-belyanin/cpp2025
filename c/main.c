#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "threadpool.h"
#include "benchmarks.h"
#include <malloc.h>

#define DEFAULT_THREADS 8
#define DEFAULT_TASKS 1048576  // 2^20
#define DEFAULT_FIB_NUMBER 30


void print_usage(const char *program_name) {
    printf("Usage: %s -b <benchmark> -t <threads> [-n <tasks>] [-f <fib_num>] [--pin] [-h]\n", program_name);
    printf("Options:\n");
    printf("  -b, --benchmark <type>  Benchmark to run: serial, parallel, fib\n");
    printf("  -t, --threads <num>     Number of worker threads (default: %d)\n", DEFAULT_THREADS);
    printf("  -n, --tasks <num>       Number of tasks for spawn benchmarks (default: %d)\n", DEFAULT_TASKS);
    printf("  -f, --fib-number <num>  Fibonacci number for fib benchmark (default: %d)\n", DEFAULT_FIB_NUMBER);
    printf("  --pin                   Pin threads to CPU cores (Linux-only)\n");
    printf("  -h, --help             Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s -b serial -t 8\n", program_name);
    printf("  %s -b parallel -t 8 -n 500000\n", program_name);
    printf("  %s -b fib -t 4 -f 35\n", program_name);
}


benchmark_type_t parse_benchmark_type(const char *type_str) {
    if (strcmp(type_str, "serial") == 0) {
        return BENCHMARK_SERIAL;
    } else if (strcmp(type_str, "parallel") == 0) {
        return BENCHMARK_PARALLEL;
    } else if (strcmp(type_str, "fib") == 0) {
        return BENCHMARK_FIBONACCI;
    } else {
        return -1; // Invalid
    }
}


const char* get_benchmark_name(benchmark_type_t type, int fib_number) {
    static char fib_name[32];
    switch (type) {
        case BENCHMARK_SERIAL: return "Serial Spawn";
        case BENCHMARK_PARALLEL: return "Parallel Spawn";
        case BENCHMARK_FIBONACCI: 
            snprintf(fib_name, sizeof(fib_name), "Fibonacci(%d)", fib_number);
            return fib_name;
        default: return "Unknown";
    }
}

int main(int argc, char *argv[]) {
    benchmark_config_t config = {
        .type = -1,
        .num_threads = DEFAULT_THREADS,
        .num_tasks = DEFAULT_TASKS,
        .fib_number = DEFAULT_FIB_NUMBER,
        .pin_threads = false
    };
    

    static struct option long_options[] = {
        {"benchmark", required_argument, 0, 'b'},
        {"threads", required_argument, 0, 't'},
        {"tasks", required_argument, 0, 'n'},
        {"fib-number", required_argument, 0, 'f'},
        {"pin", no_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    bool simple = false;
    int runs = 40;
    
    while ((opt = getopt_long(argc, argv, "b:t:n:f:phsq", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'b':
                config.type = parse_benchmark_type(optarg);
                if (config.type == -1) {
                    fprintf(stderr, "Error: Invalid benchmark type '%s'\n", optarg);
                    print_usage(argv[0]);
                    return 1;
                }
                break;
            case 't':
                config.num_threads = atoi(optarg);
                if (config.num_threads <= 0) {
                    fprintf(stderr, "Error: Number of threads must be positive\n");
                    return 1;
                }
                break;
            case 'n':
                config.num_tasks = atoi(optarg);
                if (config.num_tasks <= 0) {
                    fprintf(stderr, "Error: Number of tasks must be positive\n");
                    return 1;
                }
                break;
            case 'f':
                config.fib_number = atoi(optarg);
                if (config.fib_number < 0) {
                    fprintf(stderr, "Error: Fibonacci number must be non-negative\n");
                    return 1;
                }
                break;
            case 'p':
                config.pin_threads = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 's':
                simple = true;
                break;
            case 'q':
                runs = 5;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    

    if (config.type == -1) {
        fprintf(stderr, "Error: Benchmark type is required\n");
        print_usage(argv[0]);
        return 1;
    }
    

    int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (config.num_threads > max_cores * 2) {
        printf("Warning: Requested %d threads, but system has only %d cores\n", 
               config.num_threads, max_cores);
    }
    
    /*printf("=== Parallel Runtime Benchmark ===\n");
    printf("Benchmark: %s\n", get_benchmark_name(config.type, config.fib_number));
    printf("Threads: %d\n", config.num_threads);
    if (config.type == BENCHMARK_SERIAL || config.type == BENCHMARK_PARALLEL) {
        printf("Tasks: %d\n", config.num_tasks);
    }
    if (config.type == BENCHMARK_FIBONACCI) {
        printf("Fibonacci number: %d\n", config.fib_number);
    }
    printf("Thread pinning: %s\n", config.pin_threads ? "enabled" : "disabled");
    printf("==========================================\n\n");*/
    
    threadpool_t *pool = threadpool_create(config.num_threads);
    if (!pool) {
        fprintf(stderr, "Error: Failed to create thread pool\n");
        return 1;
    }
    

    if (config.pin_threads) {
        printf("Setting thread affinity...\n");
        for (int i = 0; i < config.num_threads; i++) {
            set_thread_affinity(pool->workers[i], i % max_cores);
        }
    }
    
    double elapsed_time = 0.0;
    
    for (int i = 0; i < runs; i++) {
        switch (config.type) {
            case BENCHMARK_SERIAL:
                elapsed_time = run_serial_spawn(pool, config.num_tasks, simple);
                break;
            case BENCHMARK_PARALLEL:
                elapsed_time = run_parallel_spawn(pool, config.num_tasks);
                break;
            case BENCHMARK_FIBONACCI:
                elapsed_time = run_fibonacci(pool, config.fib_number);
                break;
        }
        
        if (elapsed_time < 0) {
            fprintf(stderr, "Error: Benchmark execution failed\n");
            threadpool_destroy(pool);
            return 1;
        }
        printf("%.6f\n", elapsed_time);
    }


    
    //printf("\n==========================================\n");
    //printf("Benchmark completed successfully!\n");
    //printf("Elapsed time: %.3f milliseconds\n", elapsed_time * 1000);
    
    //if (config.type == BENCHMARK_SERIAL || config.type == BENCHMARK_PARALLEL) {
    //    double throughput = config.num_tasks / elapsed_time;
    //    printf("Throughput: %.0f tasks/second\n", throughput);
    //    printf("Average task time: %.3f microseconds\n", (elapsed_time * 1e6) / config.num_tasks);
    //}
    
    //printf("==========================================\n");
    
    threadpool_destroy(pool);
    
    return 0;
}
