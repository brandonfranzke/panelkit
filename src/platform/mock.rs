//! Mock platform implementation for proof-of-life testing
//!
//! This module provides mock implementations of platform-specific components for testing.

use crate::event::{Event, TouchAction};
use crate::platform::{Color, GraphicsContext, PlatformDriver, Point, Rectangle};
use anyhow::Result;
use std::time::{Duration, Instant};

/// Mock graphics context that simulates rendering operations
pub struct MockGraphicsContext {
    width: u32,
    height: u32,
    current_color: Color,
}

impl MockGraphicsContext {
    /// Create a new mock graphics context
    pub fn new(width: u32, height: u32) -> Self {
        Self {
            width,
            height,
            current_color: Color::rgb(0, 0, 0),
        }
    }
}

impl GraphicsContext for MockGraphicsContext {
    fn clear(&mut self, color: Color) -> Result<()> {
        log::trace!("Mock clear with color: RGB({}, {}, {})", color.r, color.g, color.b);
        Ok(())
    }
    
    fn set_draw_color(&mut self, color: Color) -> Result<()> {
        self.current_color = color;
        log::trace!("Mock set color: RGB({}, {}, {})", color.r, color.g, color.b);
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle) -> Result<()> {
        log::trace!("Mock fill rectangle: ({}, {}, {}, {}) with color RGB({}, {}, {})",
            rect.x, rect.y, rect.width, rect.height,
            self.current_color.r, self.current_color.g, self.current_color.b);
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle) -> Result<()> {
        log::trace!("Mock draw rectangle outline: ({}, {}, {}, {})",
            rect.x, rect.y, rect.width, rect.height);
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point) -> Result<()> {
        log::trace!("Mock draw line from ({}, {}) to ({}, {})",
            start.x, start.y, end.x, end.y);
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
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
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
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
            events.push(Event::Touch {
                x: self.x,
                y: self.y,
                action: TouchAction::Press,
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
    
    fn create_graphics_context(&mut self) -> Result<Box<dyn GraphicsContext>> {
        // Create a new mock graphics context
        let context = MockGraphicsContext::new(self.width, self.height);
        Ok(Box::new(context))
    }
    
    fn cleanup(&mut self) {
        log::info!("Mock platform cleaned up");
    }
}