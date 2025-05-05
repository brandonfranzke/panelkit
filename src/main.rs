//! PanelKit - Embedded UI Application
//! 
//! This is the main entry point for the PanelKit application.

use clap::{Parser, ValueEnum};
use log::LevelFilter;
use panelkit::{AppConfig, Application, TargetPlatform, logging};
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

/// Target platform options for the application
#[derive(Debug, Clone, Copy, ValueEnum)]
enum PlatformArg {
    /// Run on the development host using SDL2
    Host,
    
    /// Run on an embedded device using framebuffer
    Embedded,
    
    /// Auto-detect platform based on environment
    Auto,
}

impl From<PlatformArg> for TargetPlatform {
    fn from(arg: PlatformArg) -> Self {
        match arg {
            PlatformArg::Host => TargetPlatform::Host,
            PlatformArg::Embedded => TargetPlatform::Embedded,
            PlatformArg::Auto => TargetPlatform::Auto,
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
    
    /// Disable fullscreen mode (overrides --fullscreen)
    #[arg(long, default_value_t = false)]
    no_fullscreen: bool,
    
    /// Path to state database
    #[arg(short, long)]
    state_path: Option<PathBuf>,
    
    /// Target platform to use
    #[arg(short, long, value_enum, default_value_t = PlatformArg::Auto)]
    platform: PlatformArg,
}

fn main() -> anyhow::Result<()> {
    // Parse command line arguments
    let args = Args::parse();

    // Initialize logger
    logging::init(args.log_level.into());
    let logger = logging::app_logger();

    logger.info("PanelKit starting up");

    // Parse dimensions
    let dimensions = if args.dimensions == "auto" {
        // Auto-detect dimensions based on platform
        // This is a default fallback value for now.
        // In the future, this will dynamically detect the actual display dimensions
        // through platform-specific capabilities.
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
    
    logger.info(&format!("Using dimensions: {}x{}", dimensions.0, dimensions.1));
    
    // Create application config
    // If no_fullscreen is specified, it overrides fullscreen setting
    let fullscreen_setting = if args.no_fullscreen {
        false
    } else {
        args.fullscreen
    };
    
    logger.info(&format!("Fullscreen mode: {}", fullscreen_setting));
    logger.info(&format!("Platform: {:?}", args.platform));
    
    let config = AppConfig {
        width: dimensions.0,
        height: dimensions.1,
        fullscreen: fullscreen_setting,
        state_path: args.state_path,
        log_level: args.log_level.into(),
        target_platform: args.platform.into(),
    };
    
    // Initialize application
    let mut app = Application::new(config)?;
    app.init()?;
    
    // Run application
    logger.info("Starting main application loop");
    let result = app.run()?;
    
    // Clean up
    app.cleanup();
    logger.info("Application terminated");
    
    Ok(result)
}