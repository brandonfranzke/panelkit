//! PanelKit - Embedded UI Application Library
//!
//! This library provides the core functionality for PanelKit.

pub mod event;
pub mod platform;
pub mod state;
pub mod ui;

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
    display_driver: Box<dyn platform::DisplayDriver>,
    input_driver: Box<dyn platform::InputDriver>,
    running: bool,
}

impl Application {
    /// Create a new application with the given configuration
    pub fn new(config: AppConfig) -> anyhow::Result<Self> {
        let event_broker = event::EventBroker::new();
        
        let state_manager = state::StateManager::new(
            config.state_path.as_deref()
        )?;
        
        let ui_manager = ui::UIManager::new();
        
        let display_driver = platform::PlatformFactory::create_display_driver();
        let input_driver = platform::PlatformFactory::create_input_driver();
        
        Ok(Self {
            config,
            event_broker,
            state_manager,
            ui_manager,
            display_driver,
            input_driver,
            running: false,
        })
    }
    
    /// Initialize the application
    pub fn init(&mut self) -> anyhow::Result<()> {
        // Initialize platform components
        self.display_driver.init(self.config.width, self.config.height)?;
        self.input_driver.init()?;
        
        // Load state
        self.state_manager.load_all()?;
        
        // Initialize UI
        self.ui_manager.init()?;
        
        Ok(())
    }
    
    /// Run the application main loop
    pub fn run(&mut self) -> anyhow::Result<()> {
        self.running = true;
        
        log::info!("Starting main application loop");
        
        while self.running {
            // Poll for input events
            let events = self.input_driver.poll_events()?;
            
            // Process events
            for event in events {
                log::info!("Processing event: {:?}", event);
                self.event_broker.publish("input", event.clone());
                self.ui_manager.process_event(&event)?;
            }
            
            // Render UI
            self.ui_manager.render()?;
            
            // For proof-of-life, use a longer sleep time
            // In production, we would use a proper timing strategy
            std::thread::sleep(std::time::Duration::from_millis(100));
        }
        
        Ok(())
    }
    
    /// Stop the application
    pub fn stop(&mut self) {
        self.running = false;
    }
    
    /// Clean up resources
    pub fn cleanup(&mut self) {
        self.display_driver.cleanup();
        self.input_driver.cleanup();
    }
}