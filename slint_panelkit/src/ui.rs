use anyhow::Result;
use crate::config::AppConfig;
use crate::state::AppState;
use crate::MainWindow;
use std::rc::Rc;
use std::cell::RefCell;
use log::{debug, info};
use slint::{ComponentHandle, Timer, TimerMode};
use std::time::Duration;
use chrono::Local;

pub fn create_main_window(config: &AppConfig) -> Result<MainWindow> {
    let window = MainWindow::new()?;
    
    // Set window dimensions if provided
    if let Some(dimensions) = config.dimensions {
        #[cfg(not(feature = "embedded"))]
        {
            window.window().set_size(slint::PhysicalSize::new(
                dimensions.width,
                dimensions.height,
            ));
            info!("Window size set to {}x{}", dimensions.width, dimensions.height);
            debug!("Orientation: {}", if dimensions.is_portrait() { "portrait" } else { "landscape" });
        }
        #[cfg(feature = "embedded")]
        {
            let _ = dimensions; // Suppress unused variable warning on embedded
        }
    }
    
    // Create shared state
    let state = Rc::new(RefCell::new(AppState::new()));
    
    // Initialize time display immediately
    let now = Local::now();
    window.set_current_time(now.format("%H:%M:%S").to_string().into());
    window.set_current_date(now.format("%d-%b-%Y").to_string().into());
    
    // Wire up callbacks
    setup_callbacks(&window, state.clone())?;
    
    // Set up timer for time updates
    setup_time_timer(&window);
    
    Ok(window)
}

fn setup_callbacks(window: &MainWindow, state: Rc<RefCell<AppState>>) -> Result<()> {
    let state_clone = state.clone();
    let window_weak = window.as_weak();
    window.on_set_background_color(move |color| {
        state_clone.borrow_mut().set_background_color(color);
        debug!("Background color changed to: {:?}", color);
        // Update the UI background immediately
        if let Some(w) = window_weak.upgrade() {
            w.set_my_background(color);
        }
    });

    let state_clone = state.clone();
    window.on_change_page(move |page| {
        let old_page = state_clone.borrow().get_current_page();
        state_clone.borrow_mut().set_current_page(page);
        info!("Changed from page {} to page {}", old_page, page);
    });

    let state_clone = state.clone();
    let window_weak = window.as_weak();
    window.on_cycle_color(move || {
        // Generate a new random color every time
        state_clone.borrow_mut().generate_random_color();
        let new_color = state_clone.borrow().get_background_color();
        debug!("Generated random color: {:?}", new_color);
        
        // Update UI background
        if let Some(w) = window_weak.upgrade() {
            w.set_my_background(new_color);
        }
    });

    window.on_quit_app(|| {
        info!("Quit requested - Application will exit");
        std::process::exit(0);
    });

    Ok(())
}

fn setup_time_timer(window: &MainWindow) {
    let window_weak = window.as_weak();
    let timer = Timer::default();
    timer.start(TimerMode::Repeated, Duration::from_millis(1000), move || {
        if let Some(w) = window_weak.upgrade() {
            let now = Local::now();
            let time_str = now.format("%H:%M:%S").to_string();
            let date_str = now.format("%d-%b-%Y").to_string();
            w.set_current_time(time_str.into());
            w.set_current_date(date_str.into());
        }
    });
    
    // Leak the timer to keep it alive for the lifetime of the application
    std::mem::forget(timer);
}