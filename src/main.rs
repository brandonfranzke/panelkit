//! PanelKit - Embedded UI Application
//! 
//! This is the main entry point for the PanelKit application.

use clap::{Parser, ValueEnum};
use log::LevelFilter;
use panelkit::{AppConfig, Application};
use std::path::PathBuf;

/// Represents available log levels for the application
#[derive(Debug, Clone, Copy, ValueEnum)]
enum LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
}

impl From<LogLevel> for LevelFilter {
    fn from(level: LogLevel) -> Self {
        match level {
            LogLevel::Trace => LevelFilter::Trace,
            LogLevel::Debug => LevelFilter::Debug,
            LogLevel::Info => LevelFilter::Info,
            LogLevel::Warn => LevelFilter::Warn,
            LogLevel::Error => LevelFilter::Error,
        }
    }
}

/// Command line arguments for PanelKit application
#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Set the logging level
    #[arg(short, long, value_enum, default_value_t = LogLevel::Info)]
    log_level: LogLevel,

    /// Set the display dimensions (format: WIDTHxHEIGHT)
    #[arg(short, long, default_value = "auto")]
    dimensions: String,

    /// Enable fullscreen mode
    #[arg(short, long, default_value_t = false)]
    fullscreen: bool,
    
    /// Path to state database
    #[arg(short, long)]
    state_path: Option<PathBuf>,
}

fn main() -> anyhow::Result<()> {
    // Parse command line arguments
    let args = Args::parse();

    // Initialize logger
    env_logger::Builder::new()
        .filter_level(match args.log_level {
            LogLevel::Trace => log::LevelFilter::Trace,
            LogLevel::Debug => log::LevelFilter::Debug,
            LogLevel::Info => log::LevelFilter::Info,
            LogLevel::Warn => log::LevelFilter::Warn,
            LogLevel::Error => log::LevelFilter::Error,
        })
        .format_timestamp(Some(env_logger::fmt::TimestampPrecision::Millis))
        .format_module_path(false)
        .init();
    log::info!("PanelKit starting up...");

    // Parse dimensions
    let dimensions = if args.dimensions == "auto" {
        // Auto-detect dimensions
        // TODO: Implement auto-detection
        (800, 480)
    } else {
        // Parse dimensions in format WIDTHxHEIGHT
        let parts: Vec<&str> = args.dimensions.split('x').collect();
        if parts.len() != 2 {
            anyhow::bail!("Invalid dimensions format. Use WIDTHxHEIGHT format.");
        }
        
        let width = parts[0].parse::<u32>()
            .map_err(|_| anyhow::anyhow!("Invalid width value: {}", parts[0]))?;
        
        let height = parts[1].parse::<u32>()
            .map_err(|_| anyhow::anyhow!("Invalid height value: {}", parts[1]))?;
        
        (width, height)
    };
    
    log::info!("Using dimensions: {}x{}", dimensions.0, dimensions.1);
    log::info!("Fullscreen mode: {}", args.fullscreen);
    
    // Check if simulator feature is enabled
    #[cfg(feature = "simulator")]
    log::info!("Build configuration: 'simulator' feature is ENABLED");
    
    #[cfg(not(feature = "simulator"))]
    log::info!("Build configuration: 'simulator' feature is DISABLED");
    
    // Create application config
    let config = AppConfig {
        width: dimensions.0,
        height: dimensions.1,
        fullscreen: args.fullscreen,
        state_path: args.state_path,
        log_level: args.log_level.into(),
    };
    
    // Initialize application
    let mut app = Application::new(config)?;
    app.init()?;
    
    // Run application
    log::info!("PanelKit main loop starting");
    let result = app.run();
    
    // Clean up
    app.cleanup();
    log::info!("PanelKit terminated");
    
    result
}