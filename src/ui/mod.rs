//! UI module for PanelKit
//!
//! This module handles all UI-related components and rendering.

use std::collections::HashMap;

// Export UI pages
pub mod hello_page;
pub mod demo_page;

/// Represents a UI page in the application
pub trait Page {
    /// Initialize the page
    fn init(&mut self) -> anyhow::Result<()>;
    
    /// Render the page to the display
    fn render(&self) -> anyhow::Result<()>;
    
    /// Handle input events for this page
    fn handle_event(&mut self, event: &crate::event::Event) -> anyhow::Result<()>;
    
    /// Called when this page becomes active
    fn on_activate(&mut self) -> anyhow::Result<()>;
    
    /// Called when this page becomes inactive
    fn on_deactivate(&mut self) -> anyhow::Result<()>;
}

/// The UI manager that handles page navigation and rendering
pub struct UIManager {
    pages: HashMap<String, Box<dyn Page>>,
    current_page: Option<String>,
}

impl UIManager {
    /// Create a new UI manager
    pub fn new() -> Self {
        Self {
            pages: HashMap::new(),
            current_page: None,
        }
    }
    
    /// Initialize the UI system
    pub fn init(&mut self) -> anyhow::Result<()> {
        // Initialize LVGL if we're using it
        #[cfg(feature = "simulator")]
        {
            // Initialize LVGL before creating pages
            lvgl::init();
        }
        
        // Register pages
        self.register_page("hello", Box::new(hello_page::HelloPage::new()))?;
        
        // Register demo page when using simulator
        #[cfg(feature = "simulator")]
        {
            self.register_page("demo", Box::new(demo_page::DemoPage::new()))?;
            
            // Navigate to demo page in simulator mode
            self.navigate_to("demo")?;
        }
        #[cfg(not(feature = "simulator"))]
        {
            // Navigate to hello page in non-simulator mode
            self.navigate_to("hello")?;
        }
        
        Ok(())
    }
    
    /// Register a page with the UI manager
    pub fn register_page(&mut self, id: &str, mut page: Box<dyn Page>) -> anyhow::Result<()> {
        page.init()?;
        self.pages.insert(id.to_string(), page);
        log::info!("Registered page: {}", id);
        Ok(())
    }
    
    /// Navigate to a specific page
    pub fn navigate_to(&mut self, page_id: &str) -> anyhow::Result<()> {
        if !self.pages.contains_key(page_id) {
            return Err(anyhow::anyhow!("Page not found: {}", page_id));
        }
        
        // Deactivate current page
        if let Some(current) = &self.current_page {
            if current != page_id {
                if let Some(page) = self.pages.get_mut(current) {
                    page.on_deactivate()?;
                }
            }
        }
        
        // Activate new page
        self.current_page = Some(page_id.to_string());
        if let Some(page) = self.pages.get_mut(page_id) {
            page.on_activate()?;
        }
        
        log::info!("Navigated to page: {}", page_id);
        Ok(())
    }
    
    /// Process an event in the UI system
    pub fn process_event(&mut self, event: &crate::event::Event) -> anyhow::Result<()> {
        if let Some(page_id) = &self.current_page {
            if let Some(page) = self.pages.get_mut(page_id) {
                page.handle_event(event)?;
            }
        }
        Ok(())
    }
    
    /// Render the current UI state
    pub fn render(&self) -> anyhow::Result<()> {
        // Let LVGL do its work if enabled
        #[cfg(feature = "simulator")]
        {
            lvgl::task_handler();
        }
        
        // Call the current page's render method
        if let Some(page_id) = &self.current_page {
            if let Some(page) = self.pages.get(page_id) {
                page.render()?;
            }
        }
        
        Ok(())
    }
}