use std::fmt;

#[derive(Clone)]
pub enum Mode {
    Serial,
    Parallel,
    Fibonacci,
}

impl fmt::Display for Mode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Mode::Serial => write!(f, "Serial"),
            Mode::Parallel=> write!(f, "Parallel"),
            Mode::Fibonacci => write!(f, "Fibonacci"),
        }
    }
}
