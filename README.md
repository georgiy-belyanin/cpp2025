# Бенчмарк параллельной среды выполнения
Реализация бенчмарка параллельной среды выполнения с использованием POSIX потоков, основанная на наборе микро-бенчмарков Sandia. Данная реализация предоставляет три различных бенчмарка для стресс-тестирования производительности пула потоков и эффективности планирования задач.

## Описание бенчмарков

### 1. Serial Spawn
    Один поток создает ~1 миллион пустых задач, все потоки выполняют их

### 2. Parallel Spawn  
    Множество потоков создают задачи параллельно, уменьшая узкое место создания

### 3. Fibonacci (**Пока что дедлокается**)
    Вычисляет Fibonacci используя рекурсию на основе задач



### Компиляция
```bash
make all
```

### Очистка сборки
```bash
make clean
```

## Использование

### Базовый синтаксис
```bash
./c_posix_runtime_bench -b <benchmark> -t <threads> [-n <tasks>] [--pin] [-h]
```

### Опции
- `-b, --benchmark <type>`: Тип бенчмарка (serial, parallel, fib)
- `-t, --threads <num>`: Количество рабочих потоков (по умолчанию: 8)  
- `-n, --tasks <num>`: Количество задач для spawn бенчмарков (по умолчанию: 1,048,576)
- `--pin`: Включить привязку потоков к ядрам CPU (только Linux)
- `-h, --help`: Показать справочное сообщение

### Примеры

#### Бенчмарк Serial Spawn
```bash
# Настройки по умолчанию (8 потоков, 1M задач)
./c_posix_runtime_bench -b serial -t 8

# Пользовательское количество задач
./c_posix_runtime_bench -b serial -t 8 -n 500000
```

#### Бенчмарк Parallel Spawn  
```bash
# Запуск с привязкой потоков
./c_posix_runtime_bench -b parallel -t 8 --pin

# Пользовательская конфигурация
./c_posix_runtime_bench -b parallel -t 16 -n 2000000
```

#### Бенчмарк Fibonacci
```bash
# Вычисление Fibonacci(30) с использованием рекурсивных задач
./c_posix_runtime_bench -b fib -t 4

# С привязкой потоков
./c_posix_runtime_bench -b fib -t 8 --pin
```


## Быстрое тестирование

Запуск быстрых тестов для проверки функциональности:
```bash
make test
```

Запуск полного набора бенчмарков:
```bash
make benchmark
```

Запуск с привязкой потоков:
```bash
make benchmark-pinned
```

### Запуск отдельных бенчмарков

Запуск конкретных бенчмарков через Makefile:

```bash
# Отдельные бенчмарки
make serial          # Serial Spawn
make parallel        # Parallel Spawn  
make fibonacci       # Fibonacci (можно также make fib)

# С привязкой потоков
make serial-pinned
make parallel-pinned
make fib-pinned

# Большие наборы данных
make serial-large    # Serial Spawn с 2M задач
make parallel-large  # Parallel Spawn с 2M задач

# Максимальная производительность
make serial-max      # Serial Spawn с 16 потоков, 4M задач, привязка
make parallel-max    # Parallel Spawn с 16 потоков, 4M задач, привязка
make fib-max        # Fibonacci с 16 потоков, привязка
```

Просмотр всех доступных целей:
```bash
make help
```