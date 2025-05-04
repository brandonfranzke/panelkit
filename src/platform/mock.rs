//! Mock platform implementation for proof-of-life testing
//!
//! This module provides mock implementations of platform-specific components for testing.

use crate::event::{Event, TouchAction};
use crate::platform::{GraphicsContext, PlatformDriver};
use anyhow::Result;
// Standard library imports
use std::time::{Duration, Instant};
use std::any::Any;

/// Mock graphics context that does nothing
pub struct MockGraphicsContext;

impl MockGraphicsContext {
    /// Create a new mock graphics context
    pub fn new() -> Self {
        Self
    }
}

impl GraphicsContext for MockGraphicsContext {
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
    graphics_context: MockGraphicsContext,
}

impl MockDriver {
    /// Create a new mock driver
    pub fn new() -> Self {
        Self {
            width: 800,
            height: 480,
            last_event: Instant::now(),
            event_interval: Duration::from_secs(2), // Generate an event every 2 seconds
            x: 100,
            y: 100,
            x_increment: 50,
            y_increment: 30,
            graphics_context: MockGraphicsContext::new(),
        }
    }
}

impl PlatformDriver for MockDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        log::info!("Mock platform initialized with dimensions: {}x{}", width, height);
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
            if self.x < 0 || self.x > 800 {
                self.x_increment = -self.x_increment;
                self.x = self.x.clamp(0, 800);
            }
            
            if self.y < 0 || self.y > 480 {
                self.y_increment = -self.y_increment;
                self.y = self.y.clamp(0, 480);
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
        log::debug!("Mock platform presenting (no-op)");
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn graphics_context(&self) -> Option<&dyn GraphicsContext> {
        Some(&self.graphics_context)
    }
    
    fn cleanup(&mut self) {
        log::info!("Mock platform cleaned up");
    }
}

// Only the unified MockDriver is needed now