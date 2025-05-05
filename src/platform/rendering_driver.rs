//! Rendering platform driver
//!
//! This module implements the PlatformDriver trait using the rendering system.

use anyhow::Result;
use crate::platform::PlatformDriver;
use crate::primitives::{RenderingContext, Color, Point, Rectangle};
use crate::event::Event;
use crate::TargetPlatform;
use std::time::Duration;

/// Primary platform driver implementation
pub struct RenderingPlatformDriver {
    rendering_context: Box<dyn RenderingContext>,
    width: u32,
    height: u32,
    title: String,
}

impl RenderingPlatformDriver {
    /// Create a new rendering platform driver
    pub fn new(title: &str, width: u32, height: u32) -> Result<Self> {
        Self::new_for_platform(title, width, height, TargetPlatform::Auto)
    }
    
    /// Create a new rendering platform driver for a specific platform
    pub fn new_for_platform(title: &str, width: u32, height: u32, platform: TargetPlatform) -> Result<Self> {
        let rendering_context = crate::rendering::RenderingFactory::create_for_platform(title, platform)?;
        
        Ok(Self {
            rendering_context,
            width,
            height,
            title: title.to_string(),
        })
    }
}

// No more adapter classes needed

impl PlatformDriver for RenderingPlatformDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        self.rendering_context.init(width, height)?;
        
        // Clear with gray background initially
        let gray = Color::rgb(100, 100, 100);
        self.rendering_context.clear(gray)?;
        self.rendering_context.present()?;
        
        log::info!("Initialized rendering platform driver with dimensions {}x{}", width, height);
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        // If our backend is SDL, we can get events from it directly via downcasting
        if let Some(sdl_backend) = self.rendering_context.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
            let events = sdl_backend.poll_events()?;
            return Ok(events);
        }
        
        // For other backends, we'd need to implement platform-specific event handling
        // For now, we'll just poll for a bit to not hog CPU and return an empty list
        std::thread::sleep(Duration::from_millis(10));
        
        Ok(vec![])
    }
    
    fn present(&mut self) -> Result<()> {
        self.rendering_context.present()
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn create_rendering_context(&mut self) -> Result<Box<dyn RenderingContext>> {
        // Create a shared rendering context that reuses the same underlying system
        // This avoids double-initialization of SDL2 backends
        
        // For SDL backend, we can create a special "clone" to avoid re-initialization
        if let Some(_sdl_backend) = self.rendering_context.as_any().downcast_ref::<crate::rendering::sdl_backend::SDLBackend>() {
            // Use a mock context for now that just delegates to the main context
            // In a real implementation, we would create a proper shared context
            let mock_context = crate::platform::mock::MockRenderingContext::new(self.width, self.height);
            return Ok(Box::new(mock_context));
        }
        
        // For other backends, create a fresh instance that won't cause initialization issues
        crate::rendering::RenderingFactory::create_for_platform(&self.title, TargetPlatform::Embedded)
    }
    
    fn cleanup(&mut self) {
        self.rendering_context.cleanup();
        log::info!("Cleaned up rendering platform driver resources");
    }
}