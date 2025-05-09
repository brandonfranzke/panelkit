//! PanelKit - Embedded UI Application Library
//!
//! This library provides the core functionality for PanelKit.

pub mod error;
pub mod event;
pub mod logging;
pub mod primitives;
pub mod platform;
pub mod rendering;
pub mod state;
pub mod ui;

// Re-export error types and Result for convenience
pub use error::{Error, Result, AnyhowResult}; 

// Re-export primitives for convenience
pub use primitives::{Color, Point, Rectangle, TextStyle, FontSize, TextAlignment, RenderingContext, Surface};
pub use ui::traits::Renderable;

// Continue to use anyhow for context in implementation
use error::Context;

/// Target platform for the application
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum TargetPlatform {
    /// Run on the development host using SDL2
    Host,
    
    /// Run on an embedded device using framebuffer
    Embedded,
    
    /// Auto-detect platform based on environment
    Auto,
}

impl Default for TargetPlatform {
    fn default() -> Self {
        Self::Auto
    }
}

/// Application configuration
pub struct AppConfig {
    /// Display width
    pub width: u32,
    
    /// Display height
    pub height: u32,
    
    /// Whether to run in fullscreen mode
    pub fullscreen: bool,
    
    /// Path to state database (None for in-memory only)
    pub state_path: Option<std::path::PathBuf>,
    
    /// Log level
    pub log_level: log::LevelFilter,
    
    /// Target platform to use
    pub target_platform: TargetPlatform,
}

/// Core application state
pub struct Application {
    config: AppConfig,
    /// Type-safe trait-based event bus system
    event_bus: event::dispatch::EventBus,
    state_manager: state::StateManager,
    ui_manager: ui::UIManager,
    platform_driver: Box<dyn platform::PlatformDriver>,
    running: bool,
}

impl Application {
    /// Create a new application with the given configuration
    pub fn new(config: AppConfig) -> Result<Self> {
        let logger = logging::app_logger();
        logger.info("Creating new application instance");
        
        // Create new event bus
        let event_bus = event::dispatch::EventBus::new();
        
        let state_manager = state::StateManager::new(
            config.state_path.as_deref()
        ).context("Failed to initialize state manager")?;
        
        let ui_manager = ui::UIManager::new();
        
        // Create platform driver using the factory with specified target platform
        let platform_driver = platform::PlatformFactory::create(config.target_platform)
            .context("Failed to create platform driver")?;
        
        Ok(Self {
            config,
            event_bus,
            state_manager,
            ui_manager,
            platform_driver,
            running: false,
        })
    }
    
    /// Initialize the application
    pub fn init(&mut self) -> Result<()> {
        let logger = logging::app_logger();
        logger.info("Starting initialization");
        
        // Step 1: Initialize platform driver
        logger.debug("Initializing platform driver");
        self.platform_driver.init(self.config.width, self.config.height)
            .context("Failed to initialize platform driver")?;
        
        // Step 2: Create rendering context and set it for UI rendering
        logger.debug("Creating rendering context for UI");
        let rendering_context = self.platform_driver.create_rendering_context()
            .context("Failed to create rendering context")?;
        
        // Pass rendering context to UI manager
        self.ui_manager.set_rendering_context(rendering_context)
            .context("Failed to set rendering context for UI manager")?;
        
        // Step 3: Load state
        logger.debug("Loading application state");
        self.state_manager.load_all()
            .context("Failed to load application state")?;
        
        // Step 4: Initialize UI
        logger.debug("Initializing UI system");
        self.ui_manager.init()
            .context("Failed to initialize UI system")?;
        
        logger.info("Initialization complete");
        Ok(())
    }
    
    /// Run the application main loop
    pub fn run(&mut self) -> Result<()> {
        let logger = logging::app_logger();
        self.running = true;
        
        logger.info("Starting main application loop");
        
        while self.running {
            // Poll for input events
            let events = self.platform_driver.poll_events()
                .context("Failed to poll for input events")?;
            
            // Process events
            for mut event in events {
                // Check for quit events
                if let Some(custom_event) = event.as_any().downcast_ref::<event::CustomEvent>() {
                    if custom_event.name == "quit" {
                        logger.info("Received quit event, exiting application");
                        self.running = false;
                    }
                    
                    // Handle navigation events
                    if custom_event.name.starts_with("navigate:") {
                        if let Some(page_id) = custom_event.name.strip_prefix("navigate:") {
                            logger.info(&format!("Received navigation event to page: {}", page_id));
                            if let Err(e) = self.ui_manager.navigate_to(page_id) {
                                logger.error(&format!("Failed to navigate to page '{}': {}", page_id, e));
                            }
                        }
                    }
                }
                
                // Publish to the event bus
                let event_clone = event.clone_event();
                if let Err(e) = self.event_bus.publish_boxed("input", event_clone) {
                    logger.error(&format!("Error publishing event to event bus: {:#}", e));
                }
                
                // Process with the event system
                if let Err(e) = self.ui_manager.process_event(event.as_mut()) {
                    logger.error(&format!("Error processing event in UI: {:#}", e));
                }
            }
            
            // Render UI
            logger.trace("Rendering UI");
            if let Err(e) = self.ui_manager.render() {
                logger.error(&format!("Error rendering UI: {:#}", e));
            }
            
            // Present to display
            logger.trace("Presenting to display");
            if let Err(e) = self.platform_driver.present() {
                logger.error(&format!("Error presenting to display: {:#}", e));
            }
            
            // Sleep to maintain reasonable framerate
            std::thread::sleep(std::time::Duration::from_millis(16));
        }
        
        Ok(())
    }
    
    /// Stop the application
    pub fn stop(&mut self) {
        self.running = false;
        logging::app_logger().info("Application stopped");
    }
    
    /// Clean up resources
    pub fn cleanup(&mut self) {
        self.platform_driver.cleanup();
        logging::app_logger().info("Application resources cleaned up");
    }
}