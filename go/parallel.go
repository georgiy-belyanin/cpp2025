package main

import (
	"fmt"
	"time"
	"os"
	"runtime"
	"strconv"
)

var runs int = 400000
var fibc int = 20
var workers int = 8

func fib(a int) int {
	if a == 0 || a == 1 {
		return 1
	} else {
		return fib(a - 1) + fib(a - 2)
	}
}

func calc(syn chan int) {
	_ = fib(fibc);
	syn <- 0
}

func main() {
	numThreads := runtime.NumCPU()
	workers := 0
	
	if len(os.Args) > 1 {
		if threads, err := strconv.Atoi(os.Args[1]); err == nil && threads > 0 {
			numThreads = threads
			workers = numThreads * 4
		} else {
			fmt.Printf("Invalid thread count: %s, using default %d\n", os.Args[1], numThreads)
		}
	}

	// Check for limit flag
	if len(os.Args) > 2 && os.Args[2] == "-limit" {
		workers = numThreads
	}

	runtime.GOMAXPROCS(numThreads)

	for i := 0; i < 40; i++ {
		syn := make(chan int, workers)
		start := time.Now()

		for i := 0; i < workers; i++ {
			go calc(syn)
		}

		for i := 0; i < runs ; i++ {
			<-syn
			go calc(syn)
		}

		for i := 0; i < workers; i++ {
			<-syn
		}

		elapsed := time.Since(start)
		duration := elapsed.Seconds()

		fmt.Printf("%.6f\n", duration)
	}
}
