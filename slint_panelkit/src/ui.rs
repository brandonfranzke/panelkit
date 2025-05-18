use anyhow::Result;
use crate::config::AppConfig;
use crate::state::AppState;
use crate::MainWindow;
use std::rc::Rc;
use std::cell::RefCell;
use log::{debug, info};
use slint::ComponentHandle;

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
    }
    
    // Create shared state
    let state = Rc::new(RefCell::new(AppState::new()));
    
    // Wire up callbacks
    setup_callbacks(&window, state)?;
    
    Ok(window)
}

fn setup_callbacks(window: &MainWindow, state: Rc<RefCell<AppState>>) -> Result<()> {
    let state_clone = state.clone();
    window.on_set_background_color(move |color| {
        state_clone.borrow_mut().set_background_color(color);
        debug!("Background color changed to: {:?}", color);
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
        let old_color = state_clone.borrow().get_background_color();
        state_clone.borrow_mut().cycle_color();
        let new_color = state_clone.borrow().get_background_color();
        debug!("Cycled color from {:?} to {:?}", old_color, new_color);
        
        // Update UI background
        if let Some(w) = window_weak.upgrade() {
            w.set_my_background(new_color);
        }
    });

    window.on_quit_app(|| {
        info!("Quit requested - Application will exit");
        // For now, we just log. In a real application, you'd implement proper shutdown
        debug!("Note: Press ESC or Ctrl+C to exit");
    });

    Ok(())
}