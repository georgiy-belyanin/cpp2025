use crate::tasks;

pub fn serial(threads: usize, tasks: u32, task: fn()) {
    let barrier = std::sync::Arc::new(tokio::sync::Barrier::new(tasks as usize + 1));
    let rt = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(threads.into())
        .build()
        .unwrap();
    for _ in 0..tasks {
        let barrier_clone = barrier.clone();
        rt.spawn(async move {
            task();
            barrier_clone.wait().await;
        });
    }

    rt.block_on(async {
        barrier.wait().await;
    });
}

pub fn serial_with_simple(threads: usize, tasks: u32, task: fn(), simple: bool) {
    if !simple {
        let barrier = std::sync::Arc::new(tokio::sync::Barrier::new(tasks as usize + 1));
        let rt = tokio::runtime::Builder::new_multi_thread()
            .worker_threads(threads.into())
            .build()
            .unwrap();
        for _ in 0..tasks {
            let barrier_clone = barrier.clone();
            rt.spawn(async move {
                task();
                barrier_clone.wait().await;
            });
        }

        rt.block_on(async {
            barrier.wait().await;
        });
    } else {
        for _ in 0..tasks {
            task();
        }
    }
}

pub fn parallel(threads: usize, tasks: u32, task: fn()) {
    let tasks_per_thread: usize = (tasks as f32 / threads as f32) as usize;
    let remaining_tasks: usize = (tasks as f32 % threads as f32) as usize;

    let barrier = std::sync::Arc::new(tokio::sync::Barrier::new(tasks as usize + 1));
    let rt = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(threads.into())
        .build()
        .unwrap();
    for i in 0..threads {
        let mut current_tasks = tasks_per_thread;
        if i < remaining_tasks {
            current_tasks += 1;
        }
        let barrier_clone = barrier.clone();

        rt.spawn(async move {
            for _ in 0..current_tasks {
                let internal_barrier_clone = barrier_clone.clone();
                tokio::task::spawn(async move {
                    task();
                    internal_barrier_clone.wait().await;
                });
            }
        });
    }

    rt.block_on(async {
        barrier.wait().await;
    });
}

pub fn fibonacci(threads: usize, num: usize) {
    let rt = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(threads.into())
        .build()
        .unwrap();

    let handle = rt.spawn(async move {
        return tasks::fib(num).await;
    });

    let _ = rt.block_on(async {
        return handle.await.unwrap();
    });
}
