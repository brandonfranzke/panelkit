//! Framebuffer rendering backend
//!
//! This module implements the RenderingContext trait using direct framebuffer 
//! access for embedded targets. Currently this is a stub implementation.

use anyhow::Result;
use crate::primitives::{Color, Point, Rectangle, TextStyle, RenderingContext, Surface};
use std::any::Any;

/// Framebuffer implementation of the RenderingContext trait
pub struct FramebufferBackend {
    width: u32,
    height: u32,
}

impl FramebufferBackend {
    /// Create a new framebuffer backend
    pub fn new() -> Result<Self> {
        // This is a stub implementation
        log::info!("Created framebuffer backend (STUB IMPLEMENTATION)");
        
        Ok(Self {
            width: 320,
            height: 240,
        })
    }
}

impl RenderingContext for FramebufferBackend {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        log::info!("Initialized framebuffer with dimensions {}x{} (STUB IMPLEMENTATION)", width, height);
        
        Ok(())
    }
    
    fn present(&mut self) -> Result<()> {
        // Stub implementation - would flush framebuffer in real implementation
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        log::info!("Cleaned up framebuffer resources (STUB IMPLEMENTATION)");
    }
    
    fn clear(&mut self, color: Color) -> Result<()> {
        log::debug!("Clear framebuffer with color {:?} (STUB IMPLEMENTATION)", color);
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::debug!("Fill rectangle {:?} with color {:?} (STUB IMPLEMENTATION)", rect, color);
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::debug!("Draw rectangle outline {:?} with color {:?} (STUB IMPLEMENTATION)", rect, color);
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        log::debug!("Draw line from {:?} to {:?} with color {:?} (STUB IMPLEMENTATION)", start, end, color);
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        log::debug!("Draw text '{}' at {:?} with style {:?} (STUB IMPLEMENTATION)", text, position, style);
        Ok(())
    }
    
    fn draw_button(&mut self, rect: Rectangle, text: &str, _bg_color: Color, _text_color: Color, _border_color: Color) -> Result<()> {
        log::debug!("Draw button '{}' at {:?} (STUB IMPLEMENTATION)", text, rect);
        Ok(())
    }
    
    fn create_surface(&mut self, width: u32, height: u32) -> Result<Box<dyn Surface>> {
        Ok(Box::new(FramebufferSurface {
            width,
            height,
        }))
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// Framebuffer implementation of a rendering surface
pub struct FramebufferSurface {
    width: u32,
    height: u32,
}

impl Surface for FramebufferSurface {
    fn clear(&mut self, color: Color) -> Result<()> {
        log::debug!("Clear surface with color {:?} (STUB IMPLEMENTATION)", color);
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::debug!("Fill rectangle {:?} with color {:?} on surface (STUB IMPLEMENTATION)", rect, color);
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        log::debug!("Draw rectangle outline {:?} with color {:?} on surface (STUB IMPLEMENTATION)", rect, color);
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        log::debug!("Draw line from {:?} to {:?} with color {:?} on surface (STUB IMPLEMENTATION)", start, end, color);
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        log::debug!("Draw text '{}' at {:?} with style {:?} on surface (STUB IMPLEMENTATION)", text, position, style);
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}