mod config;
mod platform;
mod state;
mod ui;

use anyhow::Result;
use clap::Parser;
use log::{debug, info, LevelFilter};
use simple_logger::SimpleLogger;

use crate::config::{AppConfig, Args};
use crate::platform::PlatformAdapter;
use crate::ui::create_main_window;

slint::include_modules!();

fn main() -> Result<()> {
    // Parse command line arguments first to get debug flag
    let args = Args::parse();
    let config = AppConfig::from_args(args);
    
    // Initialize logging with formatted output
    let log_level = if config.debug_logging {
        LevelFilter::Debug
    } else {
        LevelFilter::Info
    };
    
    SimpleLogger::new()
        .with_level(log_level)
        .with_colors(true)
        .init()?;

    info!("Starting PanelKit application");
    info!("Configuration: {:?}", config);
    debug!("Platform: {}", if config.is_embedded { "embedded" } else { "desktop" });

    // Initialize platform-specific display settings
    platform::configure_display();

    // Create platform adapter
    let platform = PlatformAdapter::new(config.is_embedded)?;
    platform.check_permissions()?;

    // Create and run the UI
    let window = create_main_window(&config)?;
    
    info!("Starting UI event loop");
    window.run()?;
    
    Ok(())
}