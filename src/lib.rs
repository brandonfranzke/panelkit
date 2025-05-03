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
    #[cfg(feature = "simulator")]
    platform_driver: Box<dyn platform::DisplayDriver + platform::InputDriver>,
    #[cfg(not(feature = "simulator"))]
    display_driver: Box<dyn platform::DisplayDriver>,
    #[cfg(not(feature = "simulator"))]
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
        
        // Create platform drivers
        #[cfg(feature = "simulator")]
        {
            let platform_driver = platform::PlatformFactory::create_driver()?;
            
            Ok(Self {
                config,
                event_broker,
                state_manager,
                ui_manager,
                platform_driver,
                running: false,
            })
        }
        
        #[cfg(not(feature = "simulator"))]
        {
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
    }
    
    /// Initialize the application
    pub fn init(&mut self) -> anyhow::Result<()> {
        // Initialize platform components
        #[cfg(feature = "simulator")]
        {
            self.platform_driver.init(self.config.width, self.config.height)?;
            self.platform_driver.init()?;
        }
        
        #[cfg(not(feature = "simulator"))]
        {
            self.display_driver.init(self.config.width, self.config.height)?;
            self.input_driver.init()?;
        }
        
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
            #[cfg(feature = "simulator")]
            let events = self.platform_driver.poll_events()?;
            
            #[cfg(not(feature = "simulator"))]
            let events = self.input_driver.poll_events()?;
            
            // Process events
            for event in events {
                // Check for quit events in simulator mode
                #[cfg(feature = "simulator")]
                if let event::Event::Custom { event_type, .. } = &event {
                    if event_type == "quit" {
                        log::info!("Received quit event, exiting application");
                        self.running = false;
                    }
                }
                
                log::debug!("Processing event: {:?}", event);
                self.event_broker.publish("input", event.clone());
                self.ui_manager.process_event(&event)?;
            }
            
            // Render UI
            self.ui_manager.render()?;
            
            // Flush display
            #[cfg(feature = "simulator")]
            {
                self.platform_driver.flush(&[])?;
            }
            
            #[cfg(not(feature = "simulator"))]
            {
                self.display_driver.flush(&[])?;
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
        #[cfg(feature = "simulator")]
        {
            self.platform_driver.cleanup();
        }
        
        #[cfg(not(feature = "simulator"))]
        {
            self.display_driver.cleanup();
            self.input_driver.cleanup();
        }
    }
}