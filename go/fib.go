package main

import (
	"fmt"
	"os"
	"runtime"
	"strconv"
	"sync/atomic"
	"time"
)

// fibCtx mirrors the C fib_ctx structure
type fibCtx struct {
	n       int
	res     int64
	pending int64
	parent  *fibCtx
}

// Global worker tracking (equivalent to is_worker in C)
var activeWorkers int64

// fibSerial - equivalent to fib_serial in C with SERIAL_CUTOFF
func fibSerial(n int) int64 {
	if n < 2 {
		return int64(n)
	}
	a, b := int64(0), int64(1)
	for i := 2; i <= n; i++ {
		temp := a + b
		a = b
		b = temp
	}
	return b
}

// enqueuOrRun - equivalent to enqueue_or_run in C
// Decides whether to spawn a new goroutine or run in current context
func enqueueOrRun(fn func(*fibCtx), ctx *fibCtx, maxWorkers int64) {
	currentWorkers := atomic.LoadInt64(&activeWorkers)
	
	if currentWorkers >= maxWorkers {
		// Run in current goroutine (like calling fn(arg) directly in C)
		fn(ctx)
	} else {
		// Spawn new goroutine (equivalent to threadpool_submit)
		atomic.AddInt64(&activeWorkers, 1)
		go func() {
			defer atomic.AddInt64(&activeWorkers, -1)
			fn(ctx)
		}()
	}
}

// fibonacciFinishTask - direct translation of fibonacci_finish_task
func fibonacciFinishTask(ctx *fibCtx, maxWorkers int64) {
	if ctx.parent != nil {
		// Add result to parent (equivalent to ctx->parent->res += ctx->res)
		atomic.AddInt64(&ctx.parent.res, ctx.res)
		
		// Decrement pending counter and check if it reaches 0
		if atomic.AddInt64(&ctx.parent.pending, -1) == 0 {
			// Submit parent's finish task (equivalent to threadpool_submit)
			enqueueOrRun(func(c *fibCtx) { fibonacciFinishTask(c, maxWorkers) }, ctx.parent, maxWorkers)
		}
		// ctx is garbage collected (equivalent to free(ctx))
	}
	// Root context cleanup handled by runFibonacci
}

// fibonacciTask - direct translation of fibonacci_task
func fibonacciTask(ctx *fibCtx, maxWorkers int64) {
	// Base case: n < 2
	if ctx.n < 2 {
		ctx.res = int64(ctx.n)
		enqueueOrRun(func(c *fibCtx) { fibonacciFinishTask(c, maxWorkers) }, ctx, maxWorkers)
		return
	}
	
	// Serial cutoff optimization (equivalent to #ifdef SERIAL_CUTOFF)
	if ctx.n <= 10 {
		ctx.res = fibSerial(ctx.n)
		enqueueOrRun(func(c *fibCtx) { fibonacciFinishTask(c, maxWorkers) }, ctx, maxWorkers)
		return
	}
	
	// Create left and right contexts (equivalent to malloc)
	left := &fibCtx{
		n:       ctx.n - 1,
		res:     0,
		pending: 0,
		parent:  ctx,
	}
	
	right := &fibCtx{
		n:       ctx.n - 2,
		res:     0,
		pending: 0,
		parent:  ctx,
	}
	
	// Set pending count (equivalent to atomic_store(&ctx->pending, 2))
	atomic.StoreInt64(&ctx.pending, 2)
	
	// Submit left task and run right task
	// This mirrors the C code: enqueue_or_run(ctx->pool, fibonacci_task, left)
	enqueueOrRun(func(c *fibCtx) { fibonacciTask(c, maxWorkers) }, left, maxWorkers)
	// fibonacci_task(right) - run right task in current goroutine
	fibonacciTask(right, maxWorkers)
}

// runFibonacci - direct translation of run_fibonacci
func runFibonacci(fibNumber int, numThreads int, limitGoroutines bool) (int64, float64) {
	
	// Create root context (equivalent to calloc)
	root := &fibCtx{
		n:       fibNumber,
		res:     0,
		pending: 0,
		parent:  nil,
	}
	
	// Start timing (equivalent to clock_gettime)
	start := time.Now()
	
	// Calculate max workers based on thread count and limit flag
	var maxWorkers int64
	if limitGoroutines {
		maxWorkers = int64(numThreads) // Limit to exact thread count
	} else {
		maxWorkers = int64(numThreads * 4) // Original behavior: 4x thread count
	}
	
	// Submit initial task (equivalent to threadpool_submit)
	done := make(chan bool)
	
	atomic.AddInt64(&activeWorkers, 1)
	go func() {
		defer atomic.AddInt64(&activeWorkers, -1)
		fibonacciTask(root, maxWorkers)
		done <- true
	}()
	
	// Wait for completion (equivalent to threadpool_wait_all)
	<-done
	
	// Wait for all workers to finish
	for atomic.LoadInt64(&activeWorkers) > 0 {
		time.Sleep(time.Microsecond)
	}
	
	elapsed := time.Since(start)
	duration := elapsed.Seconds()
	
	return root.res, duration
}
var testCases = []int{45}

// Benchmark function to compare with C implementation
func benchmark(numThreads int, limitGoroutines bool) {
	for _, n := range testCases {
		// Run 20 iterations for better timing accuracy
		var totalTime float64
		iterations := 20
		
		for i := 0; i < iterations; i++ {
			_, dur := runFibonacci(n, numThreads, limitGoroutines)
			totalTime += dur
			fmt.Printf("%.6f\n", dur)
		}
	}
}

func main() {
	numThreads := runtime.NumCPU()
	limitGoroutines := false
	
	if len(os.Args) > 1 {
		if threads, err := strconv.Atoi(os.Args[1]); err == nil && threads > 0 {
			numThreads = threads
		} else {
			fmt.Printf("Invalid thread count: %s, using default %d\n", os.Args[1], numThreads)
		}
	}

	// Check for limit flag
	if len(os.Args) > 2 && os.Args[2] == "-limit" {
		limitGoroutines = true
	}
	
	runtime.GOMAXPROCS(numThreads)

	benchmark(numThreads, limitGoroutines)
		
	var m runtime.MemStats
	runtime.ReadMemStats(&m)
	fmt.Printf("%d,%d\n", m.Alloc/1024, m.TotalAlloc/1024)

	// Resulting format: threads,mean (seconds),stdenv (seconds), alloc (kb), total_alloc(kb)
}
