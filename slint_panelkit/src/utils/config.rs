use clap::Parser;
use log::LevelFilter;

/// Panel Kit application configuration
#[derive(Parser, Debug, Clone)]
#[command(author, version, about, long_about = None)]
pub struct AppConfig {
    /// Screen width in pixels
    #[arg(long, default_value_t = 640)]
    pub width: u32,
    
    /// Screen height in pixels
    #[arg(long, default_value_t = 480)]
    pub height: u32,
    
    /// Screen orientation (portrait or landscape)
    #[arg(long, default_value = "portrait")]
    pub orientation: String,
    
    /// Run in embedded mode (using LinuxKMS backend)
    #[arg(long)]
    pub embedded: bool,
    
    /// Log level (error, warn, info, debug, trace)
    #[arg(long, default_value = "info")]
    pub log_level: String,
    
    /// Enable touch event logging
    #[arg(long)]
    pub touch_logging: bool,
    
    /// Enable visual debugging overlay
    #[arg(long)]
    pub debug_overlay: bool,
}

impl AppConfig {
    /// Parse command line arguments and create a configuration
    pub fn new() -> Self {
        Self::parse()
    }
    
    /// Get the log level filter
    pub fn log_level_filter(&self) -> LevelFilter {
        match self.log_level.to_lowercase().as_str() {
            "error" => LevelFilter::Error,
            "warn" => LevelFilter::Warn,
            "info" => LevelFilter::Info,
            "debug" => LevelFilter::Debug,
            "trace" => LevelFilter::Trace,
            _ => LevelFilter::Info,
        }
    }
    
    /// Check if the screen is in portrait orientation
    pub fn is_portrait(&self) -> bool {
        self.orientation.to_lowercase() == "portrait"
    }
    
    /// Get the window width based on orientation
    pub fn window_width(&self) -> u32 {
        if self.is_portrait() { self.width } else { self.height }
    }
    
    /// Get the window height based on orientation
    pub fn window_height(&self) -> u32 {
        if self.is_portrait() { self.height } else { self.width }
    }
}