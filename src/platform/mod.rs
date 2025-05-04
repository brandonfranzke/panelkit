//! Platform abstraction for PanelKit
//!
//! This module provides platform-specific implementations for display, input, and timing.

use anyhow::Result;

// Include platform implementations
pub mod graphics;
pub mod mock;
pub mod sdl_driver;
pub mod rendering_driver;

// Re-export graphics types for convenience and transitional compatibility
pub use graphics::{GraphicsContext, Renderable};
pub use crate::rendering::primitives::{Color, Point, Rectangle, TextStyle, FontSize, TextAlignment};

/// Unified platform driver interface that handles both display and input
pub trait PlatformDriver {
    /// Initialize the driver with given dimensions
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    
    /// Poll for input events
    fn poll_events(&mut self) -> Result<Vec<crate::event::Event>>;
    
    /// Present rendered content to the display
    fn present(&mut self) -> Result<()>;
    
    /// Get the display dimensions
    fn dimensions(&self) -> (u32, u32);
    
    /// Create a graphics context for rendering
    fn create_graphics_context(&mut self) -> Result<Box<dyn GraphicsContext>>;
    
    /// Release all resources
    fn cleanup(&mut self);
}

/// Factory for creating platform-appropriate driver implementations
pub struct PlatformFactory;

impl PlatformFactory {
    /// Create a platform driver based on the current environment
    pub fn create() -> Result<Box<dyn PlatformDriver>> {
        // Default dimensions - these should be overridden by application config
        // or auto-detected when possible
        let default_width = 800;
        let default_height = 480;
        let app_title = "PanelKit";
        
        // Use our rendering abstraction-based driver
        match rendering_driver::RenderingPlatformDriver::new(app_title, default_width, default_height) {
            Ok(driver) => {
                log::info!("Using rendering abstraction-based platform driver");
                return Ok(Box::new(driver));
            },
            Err(e) => {
                log::warn!("Failed to initialize rendering platform driver: {:?}. Falling back to standard drivers.", e);
                // Fall through to standard drivers
            }
        }
        
        // Standard driver paths (fallback)
        #[cfg(feature = "host")]
        {
            // Default to SDL driver
            let sdl_driver = sdl_driver::SDLDriver::new(
                default_width, 
                default_height, 
                app_title
            )?;
            return Ok(Box::new(sdl_driver));
        }
        
        #[cfg(feature = "embedded")]
        {
            // For now, use mock driver for embedded feature
            // This will be replaced with a proper framebuffer driver
            let mock_driver = mock::MockDriver::new();
            return Ok(Box::new(mock_driver));
        }
        
        // This should be unreachable due to the cfg blocks above
        #[allow(unreachable_code)]
        {
            let mock_driver = mock::MockDriver::new();
            Ok(Box::new(mock_driver))
        }
    }
}