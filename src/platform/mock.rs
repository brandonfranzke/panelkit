//! Mock platform implementation for proof-of-life testing
//!
//! This module provides mock implementations of platform-specific components for testing.

use crate::event::{LegacyEvent, LegacyTouchAction};
use crate::platform::PlatformDriver;
use crate::primitives::{Color, Point, Rectangle, RenderingContext, TextStyle, Surface, FontSize, TextAlignment};
use anyhow::Result;
use std::time::{Duration, Instant};
use std::any::Any;

/// Mock rendering context that simulates rendering operations
pub struct MockRenderingContext {
    width: u32,
    height: u32,
}

impl MockRenderingContext {
    /// Create a new mock rendering context
    pub fn new(width: u32, height: u32) -> Self {
        Self {
            width,
            height,
        }
    }
}

impl RenderingContext for MockRenderingContext {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        log::trace!("Mock init with dimensions: {}x{}", width, height);
        Ok(())
    }
    
    fn present(&mut self) -> Result<()> {
        log::trace!("Mock present called");
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        log::trace!("Mock cleanup called");
    }
    
    fn clear(&mut self, color: Color) -> Result<()> {
        log::trace!("Mock clear with color: RGB({}, {}, {}, {})", color.r, color.g, color.b, color.a);
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::trace!("Mock fill rectangle: ({}, {}, {}, {}) with color RGB({}, {}, {}, {})",
            rect.x, rect.y, rect.width, rect.height,
            color.r, color.g, color.b, color.a);
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::trace!("Mock draw rectangle outline: ({}, {}, {}, {}) with color RGB({}, {}, {}, {})",
            rect.x, rect.y, rect.width, rect.height,
            color.r, color.g, color.b, color.a);
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        log::trace!("Mock draw line from ({}, {}) to ({}, {}) with color RGB({}, {}, {}, {})",
            start.x, start.y, end.x, end.y,
            color.r, color.g, color.b, color.a);
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        log::trace!("Mock draw text: '{}' at ({}, {})", text, position.x, position.y);
        Ok(())
    }
    
    fn draw_button(&mut self, rect: Rectangle, text: &str, bg_color: Color, text_color: Color, border_color: Color) -> Result<()> {
        log::trace!("Mock draw button: '{}' at ({}, {}, {}, {})", 
            text, rect.x, rect.y, rect.width, rect.height);
        Ok(())
    }
    
    fn create_surface(&mut self, width: u32, height: u32) -> Result<Box<dyn Surface>> {
        log::trace!("Mock create surface: {}x{}", width, height);
        Ok(Box::new(MockSurface::new(width, height)))
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// Mock surface implementation
pub struct MockSurface {
    width: u32,
    height: u32,
}

impl MockSurface {
    pub fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
}

impl Surface for MockSurface {
    fn clear(&mut self, color: Color) -> Result<()> {
        log::trace!("Mock surface clear with color: RGB({}, {}, {}, {})", color.r, color.g, color.b, color.a);
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::trace!("Mock surface fill rect at ({}, {}, {}, {})", rect.x, rect.y, rect.width, rect.height);
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::trace!("Mock surface draw rect at ({}, {}, {}, {})", rect.x, rect.y, rect.width, rect.height);
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        log::trace!("Mock surface draw line from ({}, {}) to ({}, {})", start.x, start.y, end.x, end.y);
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        log::trace!("Mock surface draw text: '{}' at ({}, {})", text, position.x, position.y);
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// Unified mock driver for the platform abstraction
pub struct MockDriver {
    width: u32,
    height: u32,
    last_event: Instant,
    event_interval: Duration,
    x: i32,
    y: i32,
    x_increment: i32,
    y_increment: i32,
}

impl MockDriver {
    /// Create a new mock driver
    pub fn new() -> Self {
        let width = 800;
        let height = 480;
        
        Self {
            width,
            height,
            last_event: Instant::now(),
            event_interval: Duration::from_secs(2), // Generate an event every 2 seconds
            x: 100,
            y: 100,
            x_increment: 50,
            y_increment: 30,
        }
    }
}

impl PlatformDriver for MockDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        // Update dimensions if provided
        if width > 0 && height > 0 {
            self.width = width;
            self.height = height;
        }
        
        log::info!("Mock platform initialized with dimensions: {}x{}", self.width, self.height);
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<LegacyEvent>> {
        let now = Instant::now();
        let mut events = Vec::new();
        
        if now.duration_since(self.last_event) >= self.event_interval {
            // Update mock touch position
            self.x += self.x_increment;
            self.y += self.y_increment;
            
            // Bounce off edges
            if self.x < 0 || self.x > self.width as i32 {
                self.x_increment = -self.x_increment;
                self.x = self.x.clamp(0, self.width as i32);
            }
            
            if self.y < 0 || self.y > self.height as i32 {
                self.y_increment = -self.y_increment;
                self.y = self.y.clamp(0, self.height as i32);
            }
            
            // Create touch event
            events.push(LegacyEvent::Touch {
                x: self.x,
                y: self.y,
                action: LegacyTouchAction::Press,
            });
            
            self.last_event = now;
            log::debug!("Generated mock touch event at ({}, {})", self.x, self.y);
        }
        
        Ok(events)
    }
    
    fn present(&mut self) -> Result<()> {
        log::trace!("Mock platform presenting (no-op)");
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn create_rendering_context(&mut self) -> Result<Box<dyn RenderingContext>> {
        // Create a new mock rendering context
        let context = MockRenderingContext::new(self.width, self.height);
        Ok(Box::new(context))
    }
    
    fn cleanup(&mut self) {
        log::info!("Mock platform cleaned up");
    }
}