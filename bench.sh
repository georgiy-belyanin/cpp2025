#/usr/bin/env sh

mkdir -p results/

for threads in 1 8 16 24; do
        echo "go/fib $threads > results/go-fib-$threads.txt"
        go/fib $threads > results/go-fib-$threads.txt
        echo "go/fib $threads > results/go-fib-$threads-limit.txt"
        go/fib $threads -limit > results/go-fib-$threads-limit.txt
        echo "c/c_posix_runtime_bench -b fib -t $threads -f 45 > results/c-fib-$threads.txt"
        c/c_posix_runtime_bench -b fib -t $threads -f 45 > results/c-fib-$threads.txt
done

for units in 10000 50000 100000; do
        echo "go/sequential $units > results/go-sequential-$units.txt"
        go/sequential $units > results/go-sequential-$units.txt
        echo "go/sequential $units -nogo > results/go-sequential-$units-nogo.txt"
        go/sequential $units -nogo > results/go-sequential-$units-nogo.txt
        echo "c/c_posix_runtime_bench -b serial -n $units > results/c-sequential-$units.txt"
        c/c_posix_runtime_bench -b serial -n $units > results/c-sequential-$units.txt
        echo "c/c_posix_runtime_bench -b serial -n $units -s > results/c-sequential-$units-simple.txt"
        c/c_posix_runtime_bench -b serial -n $units -s > results/c-sequential-$units-simple.txt
done

for threads in 1 8 16 24; do
        echo "go/parallel $threads > results/go-parallel-$threads.txt"
        go/parallel $threads > results/go-parallel-$threads.txt
        echo "go/parallel $threads -limit > results/go-parallel-$threads-limit.txt"
        go/parallel $threads -limit > results/go-parallel-$threads-limit.txt
        echo "c/c_posix_runtime_bench -b parallel -t $threads -n 400000 > results/c-parallel-$threads.txt"
        c/c_posix_runtime_bench -b parallel -t $threads -n 400000 > results/c-parallel-$threads.txt
done
