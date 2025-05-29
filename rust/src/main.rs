mod mode;
mod runners;
mod tasks;

use clap::{Args, Parser, Subcommand};
use mode::Mode;
use std::time::Instant;

#[derive(Subcommand)]
enum Commands {
    Serial(TaskArgs),
    Parallel(TaskArgs),
    Fibonacci(FibArgs),
}

fn get_mode(cmd: &Commands) -> Mode {
    match cmd {
        Commands::Serial(_) => Mode::Serial,
        Commands::Parallel(_) => Mode::Parallel,
        Commands::Fibonacci(_) => Mode::Fibonacci,
    }
}

#[derive(Args)]
struct FibArgs {
    #[arg(short, long)]
    num: usize,
}

#[derive(Args)]
struct TaskArgs {
    #[arg(short, long)]
    tasks: u32,
}

#[derive(Parser)]
#[command(version, about, long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,

    #[arg(long)]
    threads: Option<usize>,

    #[arg(short, long, default_value_t = false)]
    pin: bool,
}

fn main() {
    let args = Cli::parse();

    let mut tasks_map = std::collections::HashMap::new();

    tasks_map.insert("dummy".to_string(), tasks::dummy_task as fn());

    println!("Starting {} spawn benchmark", get_mode(&args.command));
    match &args.command {
        Commands::Serial(task_args) => {
            println!(
                "Configuration: {} threads, {} tasks",
                args.threads.unwrap_or(num_cpus::get()),
                task_args.tasks
            );
        }
        Commands::Parallel(task_args) => {
            println!(
                "Configuration: {} threads, {} tasks",
                args.threads.unwrap_or(num_cpus::get()),
                task_args.tasks
            );
        }
        Commands::Fibonacci(num_args) => {
            println!(
                "Configuration: {} threads, {} fibonacci number",
                args.threads.unwrap_or(num_cpus::get()),
                num_args.num
            );
        }
    }

    match &args.command {
        Commands::Serial(task_args) => {
            for task in tasks_map {
                std::println!("\nRunning {} function benchmark", task.0);
                let start = Instant::now();
                runners::serial(
                    args.threads.unwrap_or(num_cpus::get()),
                    task_args.tasks,
                    task.1,
                );
                let end = Instant::now();

                let time = end - start;
                std::println!(
                    "Throughput: {} tasks/s",
                    task_args.tasks as f32 / time.as_secs_f32()
                );
                std::println!("Elapsed time {}s", time.as_secs_f32());
            }
        }
        Commands::Parallel(task_args) => {
            for task in tasks_map {
                std::println!("\nRunning {} function benchmark", task.0);
                let start = Instant::now();
                runners::parallel(args.threads.unwrap_or(num_cpus::get()), task_args.tasks, task.1);
                let end = Instant::now();

                let time = end - start;
                std::println!(
                    "Throughput: {} tasks/s",
                    task_args.tasks as f32 / time.as_secs_f32()
                );
                std::println!("Elapsed time {}s", time.as_secs_f32());
            }
        }
        Commands::Fibonacci(num_args) => {
            let start = Instant::now();
            runners::fibonacci(args.threads.unwrap_or(num_cpus::get()), num_args.num);
            let end = Instant::now();

            let time = end - start;
            std::println!("Elapsed time {}s", time.as_secs_f32());
        }
    }
}

