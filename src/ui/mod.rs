//! UI module for PanelKit
//!
//! This module handles all UI-related components and rendering.

use std::collections::HashMap;
use std::any::Any;
use anyhow::{Result, Context};
use crate::error::UIError;
use crate::RenderingContext;
use crate::logging;
use crate::event::Event;

// Core trait definitions
pub mod traits;

// Export UI components
pub mod components;

// Export UI pages
pub mod simple_demo_page;

// UI Pages
pub mod hello_rendering_page;
pub mod world_rendering_page;

/// Represents a UI page in the application
pub trait Page {
    /// Initialize the page
    fn init(&mut self) -> Result<()>;
    
    /// Render the page using the provided rendering context
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()>;
    
    /// Handle events
    /// Returns a navigation target string if the page wants to navigate
    fn handle_event(&mut self, event: &mut dyn Event) -> Result<Option<String>>;
    
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

/// DO NOT implement EventHandler for Page because it would conflict with the Component impl
/// Instead, we'll directly use the handle_event method in the UIManager

/// The UI manager that handles page navigation and rendering
pub struct UIManager {
    pages: HashMap<String, Box<dyn Page>>,
    current_page: Option<String>,
    rendering_context: Option<Box<dyn RenderingContext>>,
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
            rendering_context: None,
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
    pub fn process_event(&mut self, event: &mut dyn Event) -> Result<()> {
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
        
        if self.rendering_context.is_none() {
            self.logger.warn("No rendering context available, can't render");
            return Ok(());
        }
        
        // Get a mutable reference to the rendering context
        let rendering_context = self.rendering_context.as_mut().unwrap();
        
        // Call the current page's render method
        if let Some(page_id) = &self.current_page {
            if let Some(page) = self.pages.get(page_id) {
                page.render(rendering_context.as_mut())
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
    
    /// Set the rendering context for UI drawing
    pub fn set_rendering_context(&mut self, ctx: Box<dyn RenderingContext>) -> Result<()> {
        self.logger.debug("Setting rendering context for UI manager");
        
        // Get dimensions from context
        let (width, height) = ctx.dimensions();
        self.width = width;
        self.height = height;
        
        // Store rendering context
        self.rendering_context = Some(ctx);
        
        // Update layout of all pages
        for (id, page) in &mut self.pages {
            self.logger.trace(&format!("Updating layout for page '{}'", id));
            page.update_layout(width, height)
                .with_context(|| format!("Failed to update layout for page '{}'", id))?;
        }
        
        self.logger.debug("Rendering context set successfully");
        Ok(())
    }
}