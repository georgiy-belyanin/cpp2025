use cpp2025::{mode::Mode, runners, tasks};
use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion};

fn serial(ctx: &mut Criterion) {
    let mut tasks_map = std::collections::HashMap::new();
    tasks_map.insert("dummy".to_string(), tasks::dummy_task as fn());

    measure_mode(Mode::Serial, tasks_map.clone(), ctx);
    measure_mode(Mode::Parallel, tasks_map.clone(), ctx);
    measure_fibonacci(ctx);
}

fn measure_mode(
    mode: Mode,
    tasks_map: std::collections::HashMap<std::string::String, fn()>,
    ctx: &mut criterion::Criterion,
) {
    let mut mode_group = ctx.benchmark_group(mode.to_string());
    let runner = match mode {
        Mode::Serial => runners::serial,
        Mode::Parallel => runners::parallel,
        Mode::Fibonacci => return,
    };

    for task in tasks_map {
        for tasks_amount in [
            2_i32.pow(10),
            2_i32.pow(14),
            2_i32.pow(17),
            2_i32.pow(19),
            2_i32.pow(20),
        ] {
            mode_group.sampling_mode(criterion::SamplingMode::Flat);
            mode_group.sample_size(15);
            mode_group.measurement_time(std::time::Duration::from_secs(10));
            mode_group.throughput(criterion::Throughput::Elements(tasks_amount as u64));
            mode_group.bench_with_input(
                BenchmarkId::new(task.0.clone(), tasks_amount),
                &tasks_amount,
                |b, amount| b.iter(|| runner(num_cpus::get(), *amount as u32, task.1)),
            );
        }
    }

    mode_group.finish();
}

fn measure_fibonacci(ctx: &mut criterion::Criterion) {
    let mut mode_group = ctx.benchmark_group("Fibonacci");
    for fib_num in [30, 1000, 3000, 7000] {
        mode_group.sampling_mode(criterion::SamplingMode::Flat);
        mode_group.sample_size(15);
        mode_group.measurement_time(std::time::Duration::from_secs(10));
        mode_group.bench_function(fib_num.to_string(), |b| {
            b.iter(|| runners::fibonacci(num_cpus::get(), fib_num))
        });
    }

    mode_group.finish();
}

criterion_group!(benches, serial);
criterion_main!(benches);
