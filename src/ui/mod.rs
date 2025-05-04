//! UI module for PanelKit
//!
//! This module handles all UI-related components and rendering.

use std::collections::HashMap;
use std::any::Any;
use anyhow::{Result, Context};
use crate::error::UIError;
use crate::platform::GraphicsContext;
use crate::logging;

// Export UI components
pub mod components;

// Export UI pages
pub mod hello_page;
pub mod simple_demo_page;

// UI Pages

// Core application pages
pub mod hello_rendering_page;
pub mod world_rendering_page;

// Additional implementation options under src/deprecated/

// Compatibility helper for working with both graphics and rendering types
pub mod compat {
    use crate::platform::graphics;
    use crate::rendering::primitives;
    
    /// Convert a graphics Point to a primitives Point
    pub fn point_to_rendering(p: &graphics::Point) -> primitives::Point {
        primitives::Point::new(p.x, p.y)
    }
    
    /// Convert a primitives Point to a graphics Point
    pub fn point_from_rendering(p: &primitives::Point) -> graphics::Point {
        graphics::Point::new(p.x, p.y)
    }
    
    /// Convert a graphics Rectangle to a primitives Rectangle
    pub fn rect_to_rendering(r: &graphics::Rectangle) -> primitives::Rectangle {
        primitives::Rectangle::new(r.x, r.y, r.width, r.height)
    }
    
    /// Convert a primitives Rectangle to a graphics Rectangle
    pub fn rect_from_rendering(r: &primitives::Rectangle) -> graphics::Rectangle {
        graphics::Rectangle::new(r.x, r.y, r.width, r.height)
    }
    
    /// Convert a graphics Color to a primitives Color
    pub fn color_to_rendering(c: &graphics::Color) -> primitives::Color {
        primitives::Color::rgb(c.r, c.g, c.b)
    }
    
    /// Convert a primitives Color to a graphics Color
    pub fn color_from_rendering(c: &primitives::Color) -> graphics::Color {
        graphics::Color::rgb(c.r, c.g, c.b)
    }
}

/// Represents a UI page in the application
pub trait Page {
    /// Initialize the page
    fn init(&mut self) -> Result<()>;
    
    /// Render the page using the provided graphics context
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()>;
    
    /// Handle input events for this page
    /// Returns a navigation event if the page wants to navigate
    fn handle_event(&mut self, event: &crate::event::Event) -> Result<Option<String>>;
    
    /// Called when this page becomes active
    fn on_activate(&mut self) -> Result<()>;
    
    /// Called when this page becomes inactive
    fn on_deactivate(&mut self) -> Result<()>;
    
    /// Update page layout based on new dimensions
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()>;
    
    /// Safe downcast to concrete type
    fn as_any(&self) -> &dyn Any;
    
    /// Safe mutable downcast to concrete type
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// The UI manager that handles page navigation and rendering
pub struct UIManager {
    pages: HashMap<String, Box<dyn Page>>,
    current_page: Option<String>,
    graphics_context: Option<Box<dyn GraphicsContext>>,
    width: u32,
    height: u32,
    logger: &'static logging::ComponentLogger,
}

impl UIManager {
    /// Create a new UI manager
    pub fn new() -> Self {
        Self {
            pages: HashMap::new(),
            current_page: None,
            graphics_context: None,
            width: 800,
            height: 480,
            logger: logging::ui_logger(),
        }
    }
    
    /// Initialize the UI system
    pub fn init(&mut self) -> Result<()> {
        self.logger.info("Initializing UI system");
        
        // Register UI pages
        self.logger.info("Initializing application pages");
        
        // Register Hello page
        self.register_page("hello", Box::new(hello_rendering_page::HelloRenderingPage::new()))
            .context("Failed to register hello page")?;
        
        // Register World page
        self.register_page("world", Box::new(world_rendering_page::WorldRenderingPage::new()))
            .context("Failed to register world page")?;
        
        // Start with the Hello page
        self.navigate_to("hello")
            .context("Failed to navigate to hello page")?;
        
        self.logger.info("UI system initialized successfully");
        Ok(())
    }
    
    /// Register a page with the UI manager
    pub fn register_page(&mut self, id: &str, mut page: Box<dyn Page>) -> Result<()> {
        self.logger.debug(&format!("Registering page: {}", id));
        
        // Initialize the page
        page.init()
            .with_context(|| format!("Failed to initialize page '{}'", id))?;
            
        // Update layout with current dimensions
        page.update_layout(self.width, self.height)
            .with_context(|| format!("Failed to update layout for page '{}'", id))?;
            
        self.pages.insert(id.to_string(), page);
        Ok(())
    }
    
    /// Navigate to a specific page
    pub fn navigate_to(&mut self, page_id: &str) -> Result<()> {
        self.logger.debug(&format!("Navigating to page: {}", page_id));
        
        if !self.pages.contains_key(page_id) {
            return Err(UIError::PageNotFound(page_id.to_string()).into());
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
        
        self.logger.info(&format!("Navigated to page: {}", page_id));
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
    pub fn render(&mut self) -> Result<()> {
        self.logger.trace("Starting render");
        
        if self.graphics_context.is_none() {
            self.logger.warn("No graphics context available, can't render");
            return Ok(());
        }
        
        // Get a mutable reference to the graphics context
        let graphics_context = self.graphics_context.as_mut().unwrap();
        
        // Call the current page's render method
        if let Some(page_id) = &self.current_page {
            if let Some(page) = self.pages.get(page_id) {
                page.render(graphics_context.as_mut())
                    .with_context(|| format!("Failed to render page '{}'", page_id))?;
            } else {
                self.logger.error(&format!("Current page '{}' not found in pages map!", page_id));
                return Err(UIError::PageNotFound(format!("Current page '{}' not found in pages map", page_id)).into());
            }
        } else {
            self.logger.warn("No current page set, nothing to render");
        }
        
        self.logger.trace("Render complete");
        Ok(())
    }
    
    /// Set the graphics context for rendering
    pub fn set_graphics_context(&mut self, ctx: Box<dyn GraphicsContext>) -> Result<()> {
        self.logger.debug("Setting graphics context for UI manager");
        
        // Get dimensions from context
        let (width, height) = ctx.dimensions();
        self.width = width;
        self.height = height;
        
        // Store graphics context
        self.graphics_context = Some(ctx);
        
        // Update layout of all pages
        for (id, page) in &mut self.pages {
            self.logger.trace(&format!("Updating layout for page '{}'", id));
            page.update_layout(width, height)
                .with_context(|| format!("Failed to update layout for page '{}'", id))?;
        }
        
        self.logger.debug("Graphics context set successfully");
        Ok(())
    }
}