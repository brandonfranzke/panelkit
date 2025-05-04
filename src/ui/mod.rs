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
    fn handle_event(&mut self, event: &crate::event::Event) -> Result<()>;
    
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
        // Register Hello page
        self.register_page("hello", Box::new(hello_page::HelloPage::new()))
            .context("Failed to register hello page")?;
        
        // Register Demo page if using the SDL context
        if let Some(RenderContext::SDL(_)) = &self.render_context {
            let demo_page = simple_demo_page::SimpleDemoPage::new();
            self.register_page("demo", Box::new(demo_page))
                .context("Failed to register demo page")?;
                
            self.navigate_to("demo")
                .context("Failed to navigate to demo page")?;
        } else {
            // Without SDL rendering, use the Hello page
            self.navigate_to("hello")
                .context("Failed to navigate to hello page")?;
        }
        
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
                page.handle_event(event)
                    .with_context(|| format!("Failed to process event in page '{}'", page_id))?;
            }
        }
        Ok(())
    }
    
    /// Render the current UI state
    pub fn render(&self) -> Result<()> {
        // Call the current page's render method
        if let Some(page_id) = &self.current_page {
            if let Some(page) = self.pages.get(page_id) {
                page.render()
                    .with_context(|| format!("Failed to render page '{}'", page_id))?;
            }
        }
        
        Ok(())
    }
    
    /// Set the SDL canvas for rendering (used with simulator)
    pub fn set_canvas(&mut self, canvas: std::sync::Arc<std::sync::Mutex<sdl2::render::Canvas<sdl2::video::Window>>>) -> Result<()> {
        // Store the render context
        self.render_context = Some(RenderContext::SDL(canvas.clone()));
        
        // If the UI is already initialized, update existing pages
        if let Some(page_id) = &self.current_page {
            if page_id == "demo" {
                if let Some(page) = self.pages.get_mut("demo") {
                    if let Some(demo_page) = page.as_any_mut().downcast_mut::<simple_demo_page::SimpleDemoPage>() {
                        demo_page.set_canvas(canvas);
                        log::debug!("Updated canvas for demo page");
                    } else {
                        log::warn!("Failed to downcast 'demo' page to SimpleDemoPage");
                    }
                }
            }
        }
        
        Ok(())
    }
}