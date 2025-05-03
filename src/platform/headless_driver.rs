//! Headless display and input driver
//!
//! This module provides implementations for display and input handling without requiring a display server.
//! It logs all operations to the console and simulates input events for testing.

use crate::event::{Event, TouchAction};
use crate::platform::{DisplayDriver, InputDriver};
use anyhow::Result;
use std::time::{Duration, Instant};

/// Headless display driver that logs display operations without requiring a window
pub struct HeadlessDriver {
    width: u32,
    height: u32,
    last_event: Instant,
    event_interval: Duration,
    x: i32,
    y: i32,
    is_pressed: bool,
    frame_count: u64,
}

impl HeadlessDriver {
    /// Create a new headless driver
    pub fn new(width: u32, height: u32, _title: &str) -> Result<Self> {
        println!("Initializing headless driver with dimensions: {}x{}", width, height);
        Ok(Self {
            width,
            height,
            last_event: Instant::now(),
            event_interval: Duration::from_secs(2), // Generate an event every 2 seconds
            x: 100,
            y: 100,
            is_pressed: false,
            frame_count: 0,
        })
    }
}

// Implement DisplayDriver for HeadlessDriver
impl DisplayDriver for HeadlessDriver {
    fn init(&mut self, _width: u32, _height: u32) -> Result<()> {
        println!("Headless display initialized with dimensions: {}x{}", self.width, self.height);
        Ok(())
    }
    
    fn flush(&mut self, _buffer: &[u8]) -> Result<()> {
        self.frame_count += 1;
        if self.frame_count % 60 == 0 {  // Log every 60 frames to reduce spam
            println!("Headless display flushed (frame {})", self.frame_count);
        }
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        println!("Headless display cleaned up");
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}

// Implement InputDriver for HeadlessDriver
impl InputDriver for HeadlessDriver {
    fn init(&mut self) -> Result<()> {
        println!("Headless input initialized");
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        let now = Instant::now();
        let mut events = Vec::new();
        
        // Simulate touch events at regular intervals
        if now.duration_since(self.last_event) >= self.event_interval {
            // Toggle press state
            self.is_pressed = !self.is_pressed;
            
            // Randomize position a bit
            self.x = (self.x + 50) % (self.width as i32 - 100);
            self.y = (self.y + 30) % (self.height as i32 - 100);
            
            // Create touch event
            let action = if self.is_pressed {
                TouchAction::Press
            } else {
                TouchAction::Release
            };
            
            events.push(Event::Touch {
                x: self.x,
                y: self.y,
                action,
            });
            
            println!("Generated headless touch event: {:?} at ({}, {})", action, self.x, self.y);
            self.last_event = now;
        }
        
        // Simulate a quit event after 5 minutes to avoid running forever
        if now.elapsed().as_secs() > 300 {
            println!("Sending automatic quit event after 5 minutes");
            events.push(Event::Custom {
                event_type: "quit".to_string(),
                payload: "auto-timeout".to_string(),
            });
        }
        
        Ok(events)
    }
    
    fn cleanup(&mut self) {
        println!("Headless input cleaned up");
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}

// Implement Driver trait for HeadlessDriver
impl crate::platform::Driver for HeadlessDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        DisplayDriver::init(self, width, height)
    }
    
    fn flush(&mut self, buffer: &[u8]) -> Result<()> {
        DisplayDriver::flush(self, buffer)
    }
    
    fn dimensions(&self) -> (u32, u32) {
        DisplayDriver::dimensions(self)
    }
    
    fn init_input(&mut self) -> Result<()> {
        InputDriver::init(self)
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        InputDriver::poll_events(self)
    }
    
    fn cleanup(&mut self) {
        DisplayDriver::cleanup(self);
        InputDriver::cleanup(self);
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}