//! Standardized logging for PanelKit
//!
//! This module provides a consistent logging approach for the entire application.

use std::sync::OnceLock;
use log::{LevelFilter, debug, error, info, trace, warn};

/// Initialize the application logger
pub fn init(level: LevelFilter) {
    env_logger::Builder::new()
        .filter_level(level)
        .format_timestamp(Some(env_logger::fmt::TimestampPrecision::Millis))
        .format_module_path(true)
        .init();
        
    info!("PanelKit logging initialized with level: {:?}", level);
}

/// Logger wrapper for component-based logging
pub struct ComponentLogger {
    component: &'static str,
}

impl ComponentLogger {
    /// Create a new component logger
    pub fn new(component: &'static str) -> Self {
        Self { component }
    }
    
    /// Log a trace message for this component
    pub fn trace(&self, message: &str) {
        trace!("[{}] {}", self.component, message);
    }
    
    /// Log a debug message for this component
    pub fn debug(&self, message: &str) {
        debug!("[{}] {}", self.component, message);
    }
    
    /// Log an info message for this component
    pub fn info(&self, message: &str) {
        info!("[{}] {}", self.component, message);
    }
    
    /// Log a warning message for this component
    pub fn warn(&self, message: &str) {
        warn!("[{}] {}", self.component, message);
    }
    
    /// Log an error message for this component
    pub fn error(&self, message: &str) {
        error!("[{}] {}", self.component, message);
    }
}

/// Get a logger for a specific component
pub fn get_logger(component: &'static str) -> ComponentLogger {
    ComponentLogger::new(component)
}

/// Static loggers for commonly used components
pub fn ui_logger() -> &'static ComponentLogger {
    static LOGGER: OnceLock<ComponentLogger> = OnceLock::new();
    LOGGER.get_or_init(|| ComponentLogger::new("UI"))
}

pub fn platform_logger() -> &'static ComponentLogger {
    static LOGGER: OnceLock<ComponentLogger> = OnceLock::new();
    LOGGER.get_or_init(|| ComponentLogger::new("Platform"))
}

pub fn app_logger() -> &'static ComponentLogger {
    static LOGGER: OnceLock<ComponentLogger> = OnceLock::new();
    LOGGER.get_or_init(|| ComponentLogger::new("App"))
}

pub fn state_logger() -> &'static ComponentLogger {
    static LOGGER: OnceLock<ComponentLogger> = OnceLock::new();
    LOGGER.get_or_init(|| ComponentLogger::new("State"))
}

pub fn event_logger() -> &'static ComponentLogger {
    static LOGGER: OnceLock<ComponentLogger> = OnceLock::new();
    LOGGER.get_or_init(|| ComponentLogger::new("Event"))
}

pub fn hello_logger() -> &'static ComponentLogger {
    static LOGGER: OnceLock<ComponentLogger> = OnceLock::new();
    LOGGER.get_or_init(|| ComponentLogger::new("HelloPage"))
}

pub fn demo_logger() -> &'static ComponentLogger {
    static LOGGER: OnceLock<ComponentLogger> = OnceLock::new();
    LOGGER.get_or_init(|| ComponentLogger::new("DemoPage"))
}