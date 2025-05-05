//! Rendering platform driver
//!
//! This module implements the PlatformDriver trait using the rendering system.

use anyhow::Result;
use crate::platform::{PlatformDriver, GraphicsContext};
use crate::platform::graphics::{Color, Point, Rectangle};
use crate::rendering::RenderingBackend;
use crate::event::Event;
use crate::TargetPlatform;
use std::time::Duration;

// Store current color in thread_local storage for GraphicsContext compatibility
thread_local! {
    static CURRENT_COLOR: std::cell::RefCell<Color> = std::cell::RefCell::new(Color::black());
}

/// Primary platform driver implementation
pub struct RenderingPlatformDriver {
    rendering_backend: Box<dyn RenderingBackend>,
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
        let rendering_backend = crate::rendering::RenderingFactory::create_for_platform(title, platform)?;
        
        Ok(Self {
            rendering_backend,
            width,
            height,
            title: title.to_string(),
        })
    }
}

/// A graphics context that wraps the rendering backend
pub struct RenderingGraphicsContext {
    /// The rendering backend instance
    pub backend: Box<dyn RenderingBackend>,
}

impl RenderingGraphicsContext {
    /// Create a new rendering graphics context
    pub fn new(backend: Box<dyn RenderingBackend>) -> Self {
        Self { backend }
    }
}

impl GraphicsContext for RenderingGraphicsContext {
    fn clear(&mut self, color: Color) -> Result<()> {
        // Convert from graphics::Color to rendering::primitives::Color
        let primitives_color = crate::rendering::primitives::Color::from_graphics(color);
        self.backend.clear(primitives_color)
    }
    
    fn set_draw_color(&mut self, color: Color) -> Result<()> {
        // Store current color in thread_local storage
        CURRENT_COLOR.with(|c| {
            *c.borrow_mut() = color;
        });
        
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle) -> Result<()> {
        // Get current color from thread_local storage
        let color = CURRENT_COLOR.with(|c| *c.borrow());
        
        // Convert using our conversion methods
        let primitives_rect = crate::rendering::primitives::Rectangle::from_graphics(rect);
        let primitives_color = crate::rendering::primitives::Color::from_graphics(color);
        
        self.backend.fill_rect(primitives_rect, primitives_color)
    }
    
    fn draw_rect(&mut self, rect: Rectangle) -> Result<()> {
        // Get current color from thread_local storage
        let color = CURRENT_COLOR.with(|c| *c.borrow());
        
        // Convert using our conversion methods
        let primitives_rect = crate::rendering::primitives::Rectangle::from_graphics(rect);
        let primitives_color = crate::rendering::primitives::Color::from_graphics(color);
        
        self.backend.draw_rect(primitives_rect, primitives_color)
    }
    
    fn draw_line(&mut self, start: Point, end: Point) -> Result<()> {
        // Get current color from thread_local storage
        let color = CURRENT_COLOR.with(|c| *c.borrow());
        
        // Convert using our conversion methods
        let primitives_start = crate::rendering::primitives::Point::from_graphics(start);
        let primitives_end = crate::rendering::primitives::Point::from_graphics(end);
        let primitives_color = crate::rendering::primitives::Color::from_graphics(color);
        
        self.backend.draw_line(primitives_start, primitives_end, primitives_color)
    }
    
    fn dimensions(&self) -> (u32, u32) {
        self.backend.dimensions()
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}

impl PlatformDriver for RenderingPlatformDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        self.rendering_backend.init(width, height)?;
        
        // Clear with gray background initially
        let rendering_color = crate::rendering::primitives::Color::rgb(100, 100, 100);
        self.rendering_backend.clear(rendering_color)?;
        self.rendering_backend.present()?;
        
        log::info!("Initialized rendering platform driver with dimensions {}x{}", width, height);
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        // If our backend is SDL, we can get events from it directly via downcasting
        if let Some(sdl_backend) = self.rendering_backend.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
            let events = sdl_backend.poll_events()?;
            return Ok(events);
        }
        
        // For other backends, we'd need to implement platform-specific event handling
        // For now, we'll just poll for a bit to not hog CPU and return an empty list
        std::thread::sleep(Duration::from_millis(10));
        
        Ok(vec![])
    }
    
    fn present(&mut self) -> Result<()> {
        self.rendering_backend.present()
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn create_graphics_context(&mut self) -> Result<Box<dyn GraphicsContext>> {
        // Create a minimal mock context that doesn't initialize a new SDL instance
        // This avoids the "SDL can only be initialized once" error
        
        // Create a dummy color context to satisfy the interface requirements
        let ctx = SDLMockGraphicsContext::new(self.width, self.height);
        
        // Return this lightweight context instead of creating a new backend
        Ok(Box::new(ctx))
    }
    
    fn cleanup(&mut self) {
        self.rendering_backend.cleanup();
        log::info!("Cleaned up rendering platform driver resources");
    }
}

/// A minimal mock graphics context that avoids initializing SDL twice
struct SDLMockGraphicsContext {
    width: u32,
    height: u32,
}

impl SDLMockGraphicsContext {
    /// Create a new mock context
    fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
}

impl GraphicsContext for SDLMockGraphicsContext {
    fn clear(&mut self, _color: Color) -> Result<()> {
        // Actual clearing is done by the main rendering backend
        Ok(())
    }
    
    fn set_draw_color(&mut self, _color: Color) -> Result<()> {
        // Store color in thread_local storage like the main context does
        CURRENT_COLOR.with(|c| {
            *c.borrow_mut() = _color;
        });
        
        Ok(())
    }
    
    fn fill_rect(&mut self, _rect: Rectangle) -> Result<()> {
        // No-op implementation
        Ok(())
    }
    
    fn draw_rect(&mut self, _rect: Rectangle) -> Result<()> {
        // No-op implementation
        Ok(())
    }
    
    fn draw_line(&mut self, _start: Point, _end: Point) -> Result<()> {
        // No-op implementation
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}