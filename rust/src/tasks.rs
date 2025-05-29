use tokio::sync::Mutex;
use std::sync::Arc;

pub fn dummy_task() {
    let mut var = 1;
    var += 1;
    std::hint::black_box(var);
}

pub async fn fib(n: usize) -> usize {
    let shared_values = Arc::new(Mutex::new(Vec::new()));
    for _ in 0..=n {
        shared_values
            .lock()
            .await
            .push(Arc::new(Mutex::new(Option::None)));
    }
    return fib_helper(n, shared_values.clone()).await;
}

async fn fib_helper(
    n: usize,
    results: Arc<Mutex<Vec<Arc<Mutex<Option<usize>>>>>>,
) -> usize {
    let result_mutex = { 
        results.lock().await[n].clone()
    };

    let mut result = result_mutex.lock().await;
    if result.is_some() {
        return result.unwrap();
    }

    let value = match n {
        0 => 0,
        1 => 1,
        p => {
            Box::pin(fib_helper(p - 1, results.clone())).await
                + Box::pin(fib_helper(p - 2, results.clone())).await
        }
    };

    *result = Some(value);
    return value;
}
