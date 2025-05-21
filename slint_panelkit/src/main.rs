use anyhow::{Result, Context};
use log::info;
use slint::{ComponentHandle, Timer, TimerMode, SharedString};
use std::rc::Rc;
use std::cell::RefCell;
use std::time::Duration;
use chrono::{Timelike, Datelike};

mod platform;
mod utils;

// Generated UI module
slint::include_modules!();

use platform::PlatformAdapter;
use utils::config::AppConfig;
use utils::logging::setup_logger;

struct AppState {
    // Window reference
    window: MainWindow,
    
    // Background color
    background_color: slint::Color,
    
    // Date button state
    date_inverted: bool,
}

impl AppState {
    fn new(window: MainWindow) -> Self {
        Self {
            window,
            background_color: slint::Color::from_rgb_u8(240, 240, 240),
            date_inverted: false,
        }
    }
    
    // Update time display
    fn update_time(&mut self) {
        let now = chrono::Local::now();
        
        // Format time as HH:MM:SS (24-hour format)
        let time = format!("{:02}:{:02}:{:02}", 
                           now.hour(), 
                           now.minute(), 
                           now.second());
        
        // Format date as YYYY-MMM-DD
        let date = format!("{}-{}-{:02}", 
                          now.year(), 
                          now.format("%b"), 
                          now.day());
        
        // Update UI
        self.window.set_current_time(SharedString::from(time));
        self.window.set_current_date(SharedString::from(date));
    }
    
    // Handle blue button click
    fn handle_blue_button(&mut self) {
        info!("Blue button clicked");
        
        // Set background to blue
        self.background_color = slint::Color::from_rgb_u8(41, 128, 185); // #2980b9
        self.window.set_page_background(self.background_color);
    }
    
    // Handle random button click
    fn handle_random_button(&mut self) {
        info!("Random button clicked");
        
        // Generate random RGB values
        let r = rand::random::<u8>();
        let g = rand::random::<u8>();
        let b = rand::random::<u8>();
        
        // Set background to random color
        self.background_color = slint::Color::from_rgb_u8(r, g, b);
        self.window.set_page_background(self.background_color);
        
        info!("Set background color to RGB({}, {}, {})", r, g, b);
    }
    
    // Handle date button click
    fn handle_date_button(&mut self) {
        info!("Date button clicked");
        
        // Toggle date inverted state
        self.date_inverted = !self.date_inverted;
        self.window.set_date_inverted(self.date_inverted);
    }
    
    // Handle scroll position changes with better logging
    fn handle_scroll_position(&self, x: f32, y: f32) {
        // Use debug level for frequent position updates
        log::debug!("Scroll position changed: x={:.1}, y={:.1}", x, y);
        
        // Use static to track if scrolling state has changed
        static mut LAST_Y: f32 = 0.0;
        static mut WAS_SCROLLING: bool = false;
        
        unsafe {
            // Check if we've moved significantly
            let is_scrolling = (y - LAST_Y).abs() > 5.0;
            
            // Only log state changes to avoid spamming the log
            if is_scrolling != WAS_SCROLLING {
                if is_scrolling {
                    info!("Scrolling STARTED");
                } else {
                    info!("Scrolling STOPPED at y={:.1}", y);
                }
                WAS_SCROLLING = is_scrolling;
            }
            
            LAST_Y = y;
        }
    }
}

fn main() -> Result<()> {
    // Parse command line arguments
    let config = AppConfig::new();
    
    // Setup logging
    setup_logger(config.log_level_filter(), config.touch_logging)
        .context("Failed to initialize logger")?;
    
    info!("Starting Panel Kit Application");
    info!("Screen dimensions: {}x{} ({})", 
           config.window_width(), 
           config.window_height(), 
           if config.is_portrait() { "portrait" } else { "landscape" });
    
    // Initialize platform adapter
    let platform = PlatformAdapter::new(&config)
        .context("Failed to initialize platform adapter")?;
    
    // Check platform permissions
    platform.check_permissions()
        .context("Failed to check platform permissions")?;
    
    // Configure display based on platform
    platform.configure_display(&config);
    
    // Create and show UI window
    let window = MainWindow::new()
        .context("Failed to create UI window")?;
    
    // Configure window
    window.set_window_width(config.window_width() as f32);
    window.set_window_height(config.window_height() as f32);
    window.set_debug_mode(config.debug_overlay);
    
    // Create app state
    let app_state = Rc::new(RefCell::new(AppState::new(window.as_weak().unwrap())));
    
    // Set up UI event callbacks
    let app_state_weak = Rc::downgrade(&app_state);
    window.on_blue_button_clicked(move || {
        if let Some(app_state) = app_state_weak.upgrade() {
            app_state.borrow_mut().handle_blue_button();
        }
    });
    
    let app_state_weak = Rc::downgrade(&app_state);
    window.on_random_button_clicked(move || {
        if let Some(app_state) = app_state_weak.upgrade() {
            app_state.borrow_mut().handle_random_button();
        }
    });
    
    let app_state_weak = Rc::downgrade(&app_state);
    window.on_date_button_clicked(move || {
        if let Some(app_state) = app_state_weak.upgrade() {
            app_state.borrow_mut().handle_date_button();
        }
    });
    
    let app_state_weak = Rc::downgrade(&app_state);
    window.on_log_scroll_position(move |x, y| {
        if let Some(app_state) = app_state_weak.upgrade() {
            app_state.borrow().handle_scroll_position(x, y);
        }
    });
    
    // Set up timer for updating time display
    let app_state_weak = Rc::downgrade(&app_state);
    let timer = Timer::default();
    timer.start(
        TimerMode::Repeated,
        Duration::from_secs(1),  // Update every second
        move || {
            if let Some(app_state) = app_state_weak.upgrade() {
                app_state.borrow_mut().update_time();
            }
        },
    );
    
    // Force an initial time update
    app_state.borrow_mut().update_time();
    
    // Show the window and run event loop
    window.run().context("Error in UI event loop")?;
    
    info!("Application terminated normally");
    Ok(())
}