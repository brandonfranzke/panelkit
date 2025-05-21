use log::{LevelFilter};
use simple_logger::SimpleLogger;
use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};
use std::collections::VecDeque;
use once_cell::sync::Lazy;

/// Maximum number of entries in the circular buffer
const MAX_TOUCH_LOG_ENTRIES: usize = 100;

/// Touch event types to track for debugging
#[derive(Debug, Clone, Copy)]
pub enum TouchEventType {
    Press,
    Move,
    Release,
    Click,
    LongPress,
    Scroll,
    Swipe,
}

/// Touch event log entry
#[derive(Debug, Clone)]
pub struct TouchLogEntry {
    pub event_type: TouchEventType,
    pub x: f32,
    pub y: f32,
    pub target: String,
    pub timestamp: std::time::SystemTime,
}

/// Global touch event logger
struct TouchEventLogger {
    entries: Mutex<VecDeque<TouchLogEntry>>,
    enabled: AtomicBool,
}

impl TouchEventLogger {
    fn new() -> Self {
        Self {
            entries: Mutex::new(VecDeque::with_capacity(MAX_TOUCH_LOG_ENTRIES)),
            enabled: AtomicBool::new(true),
        }
    }

    fn log(&self, entry: TouchLogEntry) {
        if self.enabled.load(Ordering::Relaxed) {
            if let Ok(mut entries) = self.entries.lock() {
                if entries.len() >= MAX_TOUCH_LOG_ENTRIES {
                    entries.pop_front();
                }
                entries.push_back(entry);
            }
        }
    }

    fn enable(&self, enabled: bool) {
        self.enabled.store(enabled, Ordering::Relaxed);
    }

    fn get_entries(&self) -> Vec<TouchLogEntry> {
        if let Ok(entries) = self.entries.lock() {
            entries.iter().cloned().collect()
        } else {
            Vec::new()
        }
    }
}

// Global touch logger instance
static TOUCH_LOGGER: Lazy<TouchEventLogger> = Lazy::new(|| TouchEventLogger::new());

/// Setup the application logger
pub fn setup_logger(log_level: LevelFilter, enable_touch_logging: bool) -> Result<(), log::SetLoggerError> {
    SimpleLogger::new()
        .with_level(log_level)
        .with_colors(true)
        .init()?;
    
    TOUCH_LOGGER.enable(enable_touch_logging);
    log::info!("Logger initialized with level: {:?}, touch logging: {}", log_level, enable_touch_logging);
    Ok(())
}

/// Log a touch event
pub fn log_touch_event(event_type: TouchEventType, x: f32, y: f32, target: &str) {
    let entry = TouchLogEntry {
        event_type,
        x,
        y,
        target: target.to_string(),
        timestamp: std::time::SystemTime::now(),
    };
    
    TOUCH_LOGGER.log(entry.clone());
    
    // Also log to regular logging system at debug level
    log::debug!(
        "Touch {:?} at ({:.1}, {:.1}) on '{}'", 
        entry.event_type, 
        entry.x, 
        entry.y, 
        entry.target
    );
}

/// Enable or disable touch logging
pub fn set_touch_logging_enabled(enabled: bool) {
    TOUCH_LOGGER.enable(enabled);
    log::info!("Touch logging {}", if enabled { "enabled" } else { "disabled" });
}

/// Get the current touch log entries
pub fn get_touch_log_entries() -> Vec<TouchLogEntry> {
    TOUCH_LOGGER.get_entries()
}