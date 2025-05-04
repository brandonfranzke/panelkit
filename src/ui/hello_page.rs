//! Hello World example page
//!
//! This is a minimal example page that displays "Hello, World!" text.

use crate::event::Event;
use crate::ui::Page;
use anyhow::Result;
use std::any::Any;

/// A simple Hello World page
pub struct HelloPage {
    counter: u32,
}

impl HelloPage {
    /// Create a new HelloPage
    pub fn new() -> Self {
        Self { counter: 0 }
    }
}

impl Page for HelloPage {
    fn init(&mut self) -> Result<()> {
        log::info!("HelloPage initialized");
        Ok(())
    }
    
    fn render(&self) -> Result<()> {
        // In a real implementation, this would use LVGL to render the page
        log::info!("HelloPage rendered with counter: {}", self.counter);
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<()> {
        // In a real app, we'd check for specific event types
        match event {
            Event::Touch { action, .. } => {
                log::info!("HelloPage received touch event: {:?}", action);
                self.counter += 1;
            }
            _ => {}
        }
        
        Ok(())
    }
    
    fn on_activate(&mut self) -> Result<()> {
        log::info!("HelloPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        log::info!("HelloPage deactivated");
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}