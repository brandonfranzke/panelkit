//! PanelKit - Embedded UI Application Library
//!
//! This library provides the core functionality for PanelKit.

pub mod event;
pub mod platform;
pub mod state;
pub mod ui;

use anyhow::{Result, Context};

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
}

/// Core application state
pub struct Application {
    config: AppConfig,
    event_broker: event::EventBroker,
    state_manager: state::StateManager,
    ui_manager: ui::UIManager,
    platform_driver: Box<dyn platform::PlatformDriver>,
    running: bool,
}

impl Application {
    /// Create a new application with the given configuration
    pub fn new(config: AppConfig) -> Result<Self> {
        let event_broker = event::EventBroker::new();
        
        let state_manager = state::StateManager::new(
            config.state_path.as_deref()
        ).context("Failed to initialize state manager")?;
        
        let ui_manager = ui::UIManager::new();
        
        // Create platform driver using the factory
        let platform_driver = platform::PlatformFactory::create()
            .context("Failed to create platform driver")?;
        
        Ok(Self {
            config,
            event_broker,
            state_manager,
            ui_manager,
            platform_driver,
            running: false,
        })
    }
    
    /// Initialize the application
    pub fn init(&mut self) -> Result<()> {
        // Initialize platform driver
        self.platform_driver.init(self.config.width, self.config.height)
            .context("Failed to initialize platform driver")?;
        
        // Get graphics context and set it for UI rendering
        if let Some(context) = self.platform_driver.graphics_context() {
            // Check if we have an SDL graphics context and pass it to the UI
            if let Some(sdl_context) = context.as_any().downcast_ref::<platform::sdl_driver::SDLGraphicsContext>() {
                self.ui_manager.set_canvas(sdl_context.canvas())
                    .context("Failed to set SDL canvas for UI manager")?;
            }
        }
        
        // Load state
        self.state_manager.load_all()
            .context("Failed to load application state")?;
        
        // Initialize UI
        self.ui_manager.init()
            .context("Failed to initialize UI system")?;
        
        Ok(())
    }
    
    /// Run the application main loop
    pub fn run(&mut self) -> Result<()> {
        self.running = true;
        
        log::info!("Starting main application loop");
        
        while self.running {
            // Poll for input events
            let events = self.platform_driver.poll_events()
                .context("Failed to poll for input events")?;
            
            // Process events
            for event in events {
                // Check for quit events
                if let event::Event::Custom { event_type, .. } = &event {
                    if event_type == "quit" {
                        log::info!("Received quit event, exiting application");
                        self.running = false;
                    }
                }
                
                log::debug!("Processing event: {:?}", event);
                self.event_broker.publish("input", event.clone());
                
                match self.ui_manager.process_event(&event) {
                    Ok(_) => {},
                    Err(e) => {
                        log::error!("Error processing event in UI: {:#}", e);
                        // Continue running despite errors in event handling
                    }
                }
            }
            
            // Render UI
            if let Err(e) = self.ui_manager.render() {
                log::error!("Error rendering UI: {:#}", e);
                // Continue rendering despite errors
            }
            
            // Present to display
            if let Err(e) = self.platform_driver.present() {
                log::error!("Error presenting to display: {:#}", e);
                // Continue running despite display errors
            }
            
            // Sleep to maintain reasonable framerate
            std::thread::sleep(std::time::Duration::from_millis(16));
        }
        
        Ok(())
    }
    
    /// Stop the application
    pub fn stop(&mut self) {
        self.running = false;
    }
    
    /// Clean up resources
    pub fn cleanup(&mut self) {
        self.platform_driver.cleanup();
        log::info!("Application resources cleaned up");
    }
}