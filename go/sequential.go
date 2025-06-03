package main

import (
	"fmt"
	"time"
	"os"
	"strconv"
)


var fibc int = 20

func fib(a int) int {
	if a == 0 || a == 1 {
		return 1
	} else {
		return fib(a - 1) + fib(a - 2)
	}
}

func main() {
	runs := 0
	goroutines := true
	
	if len(os.Args) > 1 {
		if runs_, err := strconv.Atoi(os.Args[1]); err == nil && runs_ > 0 {
			runs = runs_
		} else {
			panic("unreach")
		}
		if len(os.Args) > 2 && os.Args[2] == "-nogo" {
			goroutines = false
		}
	}

	for i := 0; i < 40; i++ {
		if goroutines {
			syn := make(chan int)
			start := time.Now()

			for i := 0; i < runs; i++ {
				go func() {
					_ = fib(fibc);
					syn <- 0
				}()
				<-syn
			}

			elapsed := time.Since(start)
			duration := elapsed.Seconds()
			fmt.Printf("%.6f\n", duration)
		} else {
			start := time.Now()

			for i := 0; i < runs; i++ {
				_ = fib(fibc);
			}

			elapsed := time.Since(start)
			duration := elapsed.Seconds()
			fmt.Printf("%.6f\n", duration)
		}
	}

	var m runtime.MemStats
	runtime.ReadMemStats(&m)
	fmt.Printf("%d,%d\n", m.Alloc/1024, m.TotalAlloc/1024)

}
