//! Rendering abstraction layer for PanelKit
//!
//! This module provides a clean abstraction over different rendering backends,
//! allowing the UI to be independent of the underlying graphics implementation.

use anyhow::Result;
use std::any::Any;

pub mod primitives;
pub mod sdl_backend;
pub mod fb_backend;

pub use primitives::{Color, Point, Rectangle, TextStyle, FontSize, TextAlignment};

/// Trait for rendering backends
///
/// This provides a unified interface for different rendering technologies (SDL2, framebuffer, etc.)
/// while abstracting away the platform-specific details.
pub trait RenderingBackend {
    /// Initialize the rendering backend with the given dimensions
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    
    /// Present the rendered content to the screen
    fn present(&mut self) -> Result<()>;
    
    /// Get the current screen dimensions
    fn dimensions(&self) -> (u32, u32);
    
    /// Clean up any resources used by the rendering backend
    fn cleanup(&mut self);
    
    /// Clear the screen with a specific color
    fn clear(&mut self, color: Color) -> Result<()>;
    
    /// Draw a filled rectangle
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a rectangle outline
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a line between two points
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()>;
    
    /// Draw text at a specific position
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()>;
    
    /// Draw a button with text
    fn draw_button(&mut self, rect: Rectangle, text: &str, bg_color: Color, text_color: Color, border_color: Color) -> Result<()>;
    
    /// Create a new surface/canvas (for offscreen rendering)
    fn create_surface(&mut self, width: u32, height: u32) -> Result<Box<dyn Surface>>;
    
    /// Cast to Any for downcasting to specific backend type if needed
    fn as_any(&self) -> &dyn Any;
    
    /// Cast to Any for downcasting to specific backend type if needed
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// Trait for rendering surfaces
///
/// A surface is a canvas that can be rendered to and then composited onto the main screen.
/// This allows for more complex rendering operations.
pub trait Surface {
    /// Clear the surface with a specific color
    fn clear(&mut self, color: Color) -> Result<()>;
    
    /// Get the dimensions of the surface
    fn dimensions(&self) -> (u32, u32);
    
    /// Draw a filled rectangle on the surface
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a rectangle outline on the surface
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a line between two points on the surface
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()>;
    
    /// Draw text at a specific position on the surface
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()>;
    
    /// Cast to Any for downcasting to specific surface type if needed
    fn as_any(&self) -> &dyn Any;
    
    /// Cast to Any for downcasting to specific surface type if needed
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// Factory for creating rendering backends based on environment
pub struct RenderingFactory;

impl RenderingFactory {
    /// Create a new rendering backend appropriate for the current environment
    pub fn create(title: &str) -> Result<Box<dyn RenderingBackend>> {
        #[cfg(feature = "host")]
        {
            let backend = sdl_backend::SDLBackend::new(title)?;
            return Ok(Box::new(backend));
        }
        
        #[cfg(feature = "embedded")]
        {
            let backend = fb_backend::FramebufferBackend::new()?;
            return Ok(Box::new(backend));
        }
        
        // This should be unreachable due to the cfg blocks above
        #[allow(unreachable_code)]
        {
            anyhow::bail!("No rendering backend available for current configuration")
        }
    }
}