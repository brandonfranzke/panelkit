//! UI module for PanelKit
//!
//! This module handles all UI-related components and rendering.

use std::collections::HashMap;
use std::any::Any;
use anyhow::{Result, Context};

// Export UI pages
pub mod hello_page;
pub mod simple_demo_page;

/// Represents a UI page in the application
pub trait Page {
    /// Initialize the page
    fn init(&mut self) -> Result<()>;
    
    /// Render the page to the display
    fn render(&self) -> Result<()>;
    
    /// Handle input events for this page
    /// Returns a navigation event if the page wants to navigate
    fn handle_event(&mut self, event: &crate::event::Event) -> Result<Option<String>>;
    
    /// Called when this page becomes active
    fn on_activate(&mut self) -> Result<()>;
    
    /// Called when this page becomes inactive
    fn on_deactivate(&mut self) -> Result<()>;

    /// Safe downcast to concrete type
    fn as_any(&self) -> &dyn Any;
    
    /// Safe mutable downcast to concrete type
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// UI renderer context for platform-specific rendering
pub enum RenderContext {
    /// SDL2 rendering context
    SDL(std::sync::Arc<std::sync::Mutex<sdl2::render::Canvas<sdl2::video::Window>>>),
    /// Mock rendering context (no-op)
    Mock,
}

/// The UI manager that handles page navigation and rendering
pub struct UIManager {
    pages: HashMap<String, Box<dyn Page>>,
    current_page: Option<String>,
    render_context: Option<RenderContext>,
}

impl UIManager {
    /// Create a new UI manager
    pub fn new() -> Self {
        Self {
            pages: HashMap::new(),
            current_page: None,
            render_context: None,
        }
    }
    
    /// Initialize the UI system
    pub fn init(&mut self) -> Result<()> {
        log::info!("UIManager: Beginning initialization...");
        
        // Register Hello page
        log::info!("UIManager: Registering 'hello' page...");
        self.register_page("hello", Box::new(hello_page::HelloPage::new()))
            .context("Failed to register hello page")?;
        log::info!("UIManager: 'hello' page registered successfully");
        
        // Check render context status
        log::info!("UIManager: Checking render context...");
        if let Some(ctx) = &self.render_context {
            match ctx {
                RenderContext::SDL(_) => log::info!("UIManager: Found SDL render context"),
                RenderContext::Mock => log::info!("UIManager: Found Mock render context"),
            }
        } else {
            log::warn!("UIManager: No render context available!");
        }
        
        // Register Demo page if using the SDL context
        if let Some(RenderContext::SDL(_)) = &self.render_context {
            log::info!("UIManager: Registering 'demo' page...");
            let demo_page = simple_demo_page::SimpleDemoPage::new();
            self.register_page("demo", Box::new(demo_page))
                .context("Failed to register demo page")?;
            log::info!("UIManager: 'demo' page registered successfully");
                
            log::info!("UIManager: Navigating to 'demo' page...");
            self.navigate_to("demo")
                .context("Failed to navigate to demo page")?;
            log::info!("UIManager: Navigation to 'demo' page successful");
        } else {
            // Without SDL rendering, use the Hello page
            log::info!("UIManager: No SDL context, navigating to 'hello' page...");
            self.navigate_to("hello")
                .context("Failed to navigate to hello page")?;
            log::info!("UIManager: Navigation to 'hello' page successful");
        }
        
        log::info!("UIManager: Initialization complete");
        Ok(())
    }
    
    /// Register a page with the UI manager
    pub fn register_page(&mut self, id: &str, mut page: Box<dyn Page>) -> Result<()> {
        page.init()
            .with_context(|| format!("Failed to initialize page '{}'", id))?;
            
        self.pages.insert(id.to_string(), page);
        log::info!("Registered page: {}", id);
        Ok(())
    }
    
    /// Navigate to a specific page
    pub fn navigate_to(&mut self, page_id: &str) -> Result<()> {
        if !self.pages.contains_key(page_id) {
            return Err(anyhow::anyhow!("Page not found: {}", page_id));
        }
        
        // Deactivate current page
        if let Some(current) = &self.current_page {
            if current != page_id {
                if let Some(page) = self.pages.get_mut(current) {
                    page.on_deactivate()
                        .with_context(|| format!("Failed to deactivate page '{}'", current))?;
                }
            }
        }
        
        // Activate new page
        self.current_page = Some(page_id.to_string());
        if let Some(page) = self.pages.get_mut(page_id) {
            page.on_activate()
                .with_context(|| format!("Failed to activate page '{}'", page_id))?;
        }
        
        log::info!("Navigated to page: {}", page_id);
        Ok(())
    }
    
    /// Process an event in the UI system
    pub fn process_event(&mut self, event: &crate::event::Event) -> Result<()> {
        if let Some(page_id) = &self.current_page {
            if let Some(page) = self.pages.get_mut(page_id) {
                // Handle the event and check for navigation requests
                let navigation = page.handle_event(event)
                    .with_context(|| format!("Failed to process event in page '{}'", page_id))?;
                
                // If the page wants to navigate, do it
                if let Some(target_page) = navigation {
                    self.navigate_to(&target_page)
                        .with_context(|| format!("Failed to navigate to page '{}'", target_page))?;
                }
            }
        }
        Ok(())
    }
    
    /// Render the current UI state
    pub fn render(&self) -> Result<()> {
        log::info!("UIManager: Starting render");
        
        // Check if we have a render context
        match &self.render_context {
            Some(ctx) => log::info!("UIManager: Render context found"),
            None => {
                log::error!("UIManager: No render context available - UI cannot render!");
                return Ok(());
            }
        }
        
        // Check if we have a current page
        if let Some(page_id) = &self.current_page {
            log::info!("UIManager: Current page is '{}'", page_id);
            
            if let Some(page) = self.pages.get(page_id) {
                log::info!("UIManager: Found page '{}', rendering...", page_id);
                
                match page.render() {
                    Ok(_) => log::info!("UIManager: Page '{}' rendered successfully", page_id),
                    Err(e) => log::error!("UIManager: Failed to render page '{}': {}", page_id, e),
                }
            } else {
                log::error!("UIManager: Current page '{}' not found in pages map!", page_id);
            }
        } else {
            log::error!("UIManager: No current page set!");
        }
        
        Ok(())
    }
    
    /// Set the SDL canvas for rendering (used with simulator)
    pub fn set_canvas(&mut self, canvas: std::sync::Arc<std::sync::Mutex<sdl2::render::Canvas<sdl2::video::Window>>>) -> Result<()> {
        log::info!("UI Manager: Setting SDL canvas for rendering");
        // Store the render context
        self.render_context = Some(RenderContext::SDL(canvas.clone()));
        
        // If the UI is already initialized, update existing pages
        if let Some(page) = self.pages.get_mut("demo") {
            if let Some(demo_page) = page.as_any_mut().downcast_mut::<simple_demo_page::SimpleDemoPage>() {
                demo_page.set_canvas(canvas.clone());
                log::debug!("Updated canvas for demo page");
            } else {
                log::warn!("Failed to downcast 'demo' page to SimpleDemoPage");
            }
        }
        
        if let Some(page) = self.pages.get_mut("hello") {
            if let Some(hello_page) = page.as_any_mut().downcast_mut::<hello_page::HelloPage>() {
                hello_page.set_canvas(canvas.clone());
                log::debug!("Updated canvas for hello page");
            } else {
                log::warn!("Failed to downcast 'hello' page to HelloPage");
            }
        }
        
        Ok(())
    }
}