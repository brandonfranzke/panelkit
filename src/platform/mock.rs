//! Mock platform implementation for proof-of-life testing
//!
//! This module provides mock implementations of platform-specific components for testing.

use crate::event::{Event, TouchAction};
use crate::platform::{DisplayDriver, InputDriver};
use anyhow::Result;
use std::time::{Duration, Instant};

/// Mock display driver that logs render calls but doesn't actually render anything
pub struct MockDisplayDriver {
    width: u32,
    height: u32,
}

impl MockDisplayDriver {
    /// Create a new mock display driver
    pub fn new() -> Self {
        Self {
            width: 800,
            height: 480,
        }
    }
}

impl DisplayDriver for MockDisplayDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        log::info!("Mock display initialized with dimensions: {}x{}", width, height);
        Ok(())
    }
    
    fn flush(&mut self, _buffer: &[u8]) -> Result<()> {
        log::debug!("Mock display flushed");
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        log::info!("Mock display cleaned up");
    }
}

/// Mock input driver that periodically generates touch events
pub struct MockInputDriver {
    last_event: Instant,
    event_interval: Duration,
    x: i32,
    y: i32,
    x_increment: i32,
    y_increment: i32,
}

impl MockInputDriver {
    /// Create a new mock input driver
    pub fn new() -> Self {
        Self {
            last_event: Instant::now(),
            event_interval: Duration::from_secs(2), // Generate an event every 2 seconds
            x: 100,
            y: 100,
            x_increment: 50,
            y_increment: 30,
        }
    }
}

impl InputDriver for MockInputDriver {
    fn init(&mut self) -> Result<()> {
        log::info!("Mock input driver initialized");
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
    
    fn cleanup(&mut self) {
        log::info!("Mock input driver cleaned up");
    }
}