// Debug logging helper for tracing SDL and UI operations
// This file is NOT meant to be part of the final application

use std::sync::atomic::{AtomicUsize, Ordering};
use std::fs::OpenOptions;
use std::io::Write;
use std::sync::Mutex;
use std::time::{SystemTime, UNIX_EPOCH};
use once_cell::sync::Lazy;

// Global operation counter to track sequence of operations
static OPERATION_COUNT: AtomicUsize = AtomicUsize::new(0);

// Global log file handle
static LOG_FILE: Lazy<Mutex<std::fs::File>> = Lazy::new(|| {
    let file = OpenOptions::new()
        .create(true)
        .write(true)
        .truncate(true)
        .open("panelkit_debug.log")
        .expect("Failed to open debug log file");
    
    Mutex::new(file)
});

// Debug log levels
pub enum LogLevel {
    Error,
    Warning,
    Info,
    Debug,
    Trace,
}

impl LogLevel {
    fn as_str(&self) -> &'static str {
        match self {
            LogLevel::Error => "ERROR",
            LogLevel::Warning => "WARN",
            LogLevel::Info => "INFO",
            LogLevel::Debug => "DEBUG",
            LogLevel::Trace => "TRACE",
        }
    }
}

// Log a message to both stdout and file
pub fn log(level: LogLevel, component: &str, message: &str) {
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_millis();
    
    let count = OPERATION_COUNT.fetch_add(1, Ordering::SeqCst);
    
    let log_message = format!(
        "[{:010}] {:5} [{:04}] {:<15} - {}\n", 
        now, 
        level.as_str(), 
        count,
        component,
        message
    );
    
    // Print to stdout
    print!("{}", log_message);
    
    // Also write to file
    if let Ok(mut file) = LOG_FILE.lock() {
        let _ = file.write_all(log_message.as_bytes());
        let _ = file.flush();
    }
}

// Helper functions for specific log levels
pub fn error(component: &str, message: &str) {
    log(LogLevel::Error, component, message);
}

pub fn warning(component: &str, message: &str) {
    log(LogLevel::Warning, component, message);
}

pub fn info(component: &str, message: &str) {
    log(LogLevel::Info, component, message);
}

pub fn debug(component: &str, message: &str) {
    log(LogLevel::Debug, component, message);
}

pub fn trace(component: &str, message: &str) {
    log(LogLevel::Trace, component, message);
}

// Helper for tracing SDL operations
pub fn trace_sdl<T, F>(component: &str, operation: &str, func: F) -> T 
where
    F: FnOnce() -> T,
{
    trace(component, &format!("START: {}", operation));
    let result = func();
    trace(component, &format!("END: {}", operation));
    result
}

// Helper for debugging SDL canvas operations
pub fn trace_canvas<T, F>(component: &str, operation: &str, func: F) -> Result<T, String> 
where
    F: FnOnce() -> Result<T, String>,
{
    trace(component, &format!("CANVAS OP START: {}", operation));
    let result = func();
    match &result {
        Ok(_) => trace(component, &format!("CANVAS OP SUCCESS: {}", operation)),
        Err(e) => error(component, &format!("CANVAS OP FAILED: {} - Error: {}", operation, e)),
    }
    result
}