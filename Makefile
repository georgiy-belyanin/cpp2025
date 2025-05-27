CC = gcc
CFLAGS = -O2 -Wall -pthread -std=c11
# Add -DSERIAL_CUTOFF to enable serial cutoff optimization for Fibonacci
# CFLAGS += -DSERIAL_CUTOFF
TARGET = c_posix_runtime_bench
OBJS = main.o threadpool.o benchmarks.o

# Default target
all: $(TARGET)

# Build with serial cutoff optimization for Fibonacci
serial-cutoff: CFLAGS += -DSERIAL_CUTOFF
serial-cutoff: clean $(TARGET)

# Build and test with serial cutoff optimization
test-serial-cutoff: serial-cutoff
	@echo "Testing Fibonacci benchmark with serial cutoff optimization..."
	./$(TARGET) -b fib -t 4 -f 20

# Compare performance with and without serial cutoff
compare-cutoff: $(TARGET)
	@echo "=== Performance Comparison: Fibonacci(25) ===" 
	@echo "Without serial cutoff:"
	./$(TARGET) -b fib -t 4 -f 30
	@echo ""
	@echo "Building with serial cutoff..."
	@$(MAKE) serial-cutoff > /dev/null 2>&1
	@echo "With serial cutoff:"
	./$(TARGET) -b fib -t 4 -f 30
	@echo ""
	@echo "Rebuilding normal version..."
	@$(MAKE) clean > /dev/null 2>&1
	@$(MAKE) $(TARGET) > /dev/null 2>&1

# Build the main executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) *.o

# Run tests
test: $(TARGET)
	@echo "Running quick tests..."
	./$(TARGET) -b serial -t 4 -n 1000
	./$(TARGET) -b parallel -t 4 -n 1000
	./$(TARGET) -b fib -t 4 -f 6

# Run benchmarks with default settings
benchmark: $(TARGET)
	@echo "Running all benchmarks with default settings..."
	./$(TARGET) -b serial -t 8
	./$(TARGET) -b parallel -t 8
	./$(TARGET) -b fib -t 8

# Run benchmarks with thread pinning
benchmark-pinned: $(TARGET)
	@echo "Running all benchmarks with thread pinning..."
	./$(TARGET) -b serial -t 8 --pin
	./$(TARGET) -b parallel -t 8 --pin
	./$(TARGET) -b fib -t 8 --pin

# Individual benchmark targets
serial: $(TARGET)
	@echo "Running Serial Spawn benchmark..."
	./$(TARGET) -b serial -t 8

parallel: $(TARGET)
	@echo "Running Parallel Spawn benchmark..."
	./$(TARGET) -b parallel -t 8

fibonacci: $(TARGET)
	@echo "Running Fibonacci benchmark..."
	./$(TARGET) -b fib -t 8

fib: fibonacci

# Individual benchmark targets with thread pinning
serial-pinned: $(TARGET)
	@echo "Running Serial Spawn benchmark with thread pinning..."
	./$(TARGET) -b serial -t 8 --pin

parallel-pinned: $(TARGET)
	@echo "Running Parallel Spawn benchmark with thread pinning..."
	./$(TARGET) -b parallel -t 8 --pin

fibonacci-pinned: $(TARGET)
	@echo "Running Fibonacci benchmark with thread pinning..."
	./$(TARGET) -b fib -t 8 --pin

fib-pinned: fibonacci-pinned

# Benchmarks with custom parameters
serial-large: $(TARGET)
	@echo "Running Serial Spawn benchmark with large task count..."
	./$(TARGET) -b serial -t 8 -n 2000000

parallel-large: $(TARGET)
	@echo "Running Parallel Spawn benchmark with large task count..."
	./$(TARGET) -b parallel -t 8 -n 2000000

# High-performance configurations
serial-max: $(TARGET)
	@echo "Running Serial Spawn benchmark with maximum performance settings..."
	./$(TARGET) -b serial -t 16 -n 4000000 --pin

parallel-max: $(TARGET)
	@echo "Running Parallel Spawn benchmark with maximum performance settings..."
	./$(TARGET) -b parallel -t 16 -n 4000000 --pin

fib-max: $(TARGET)
	@echo "Running Fibonacci benchmark with maximum performance settings..."
	./$(TARGET) -b fib -t 16 --pin

# Help target
help:
	@echo "Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  all            - Build the benchmark executable (default)"
	@echo "  serial-cutoff  - Build with Fibonacci serial cutoff optimization"
	@echo "  clean          - Remove build artifacts"
	@echo ""
	@echo "Serial cutoff testing:"
	@echo "  test-serial-cutoff - Build with serial cutoff and run test"
	@echo "  compare-cutoff - Compare performance with/without serial cutoff"
	@echo ""
	@echo "Test targets:"
	@echo "  test           - Run quick tests"
	@echo ""
	@echo "Benchmark suites:"
	@echo "  benchmark      - Run all benchmarks with default settings"
	@echo "  benchmark-pinned - Run all benchmarks with thread pinning"
	@echo ""
	@echo "Individual benchmarks:"
	@echo "  serial         - Run Serial Spawn benchmark"
	@echo "  parallel       - Run Parallel Spawn benchmark" 
	@echo "  fibonacci (fib) - Run Fibonacci benchmark"
	@echo ""
	@echo "Individual benchmarks with thread pinning:"
	@echo "  serial-pinned  - Run Serial Spawn with thread pinning"
	@echo "  parallel-pinned - Run Parallel Spawn with thread pinning"
	@echo "  fibonacci-pinned (fib-pinned) - Run Fibonacci with thread pinning"
	@echo ""
	@echo "Large-scale benchmarks:"
	@echo "  serial-large   - Run Serial Spawn with 2M tasks"
	@echo "  parallel-large - Run Parallel Spawn with 2M tasks"
	@echo ""
	@echo "Maximum performance benchmarks:"
	@echo "  serial-max     - Run Serial Spawn with max settings"
	@echo "  parallel-max   - Run Parallel Spawn with max settings"
	@echo "  fib-max        - Run Fibonacci with max settings"
	@echo ""
	@echo "  help           - Show this help message"

.PHONY: all clean test benchmark benchmark-pinned help \
        serial-cutoff test-serial-cutoff compare-cutoff \
        serial parallel fibonacci fib \
        serial-pinned parallel-pinned fibonacci-pinned fib-pinned \
        serial-large parallel-large \
        serial-max parallel-max fib-max
