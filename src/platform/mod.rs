//! Platform abstraction for PanelKit
//!
//! This module provides platform-specific implementations for display, input, and timing.

use anyhow::{Result, Context};
use crate::TargetPlatform;

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
    /// Create a platform driver based on the specified target platform
    pub fn create(target_platform: TargetPlatform) -> Result<Box<dyn PlatformDriver>> {
        // Default dimensions - these should be overridden by application config
        // or auto-detected when possible
        let default_width = 800;
        let default_height = 480;
        let app_title = "PanelKit";
        
        // IMPORTANT: Only use one driver implementation - don't try to create multiples
        // as SDL2 can only be initialized once per application
        
        // Auto-detect platform if requested
        let platform = if target_platform == TargetPlatform::Auto {
            Self::detect_platform()
        } else {
            target_platform
        };
        
        log::info!("Creating platform driver for target: {:?}", platform);
        
        match platform {
            TargetPlatform::Host => {
                // Try to create rendering driver with SDL2 backend
                Self::create_host_driver(app_title, default_width, default_height)
            },
            TargetPlatform::Embedded => {
                // Create embedded driver or fallback to mock
                Self::create_embedded_driver()
            },
            TargetPlatform::Auto => {
                // This should be unreachable since we resolve Auto above
                unreachable!("Auto platform should have been resolved")
            }
        }
    }
    
    /// Detect the current platform
    fn detect_platform() -> TargetPlatform {
        // Check if we're likely on an embedded device
        let is_embedded = cfg!(target_arch = "arm") || 
                         std::env::var("PANELKIT_EMBEDDED").is_ok() ||
                         std::path::Path::new("/dev/fb0").exists();
        
        if is_embedded {
            log::info!("Auto-detected embedded platform");
            TargetPlatform::Embedded
        } else {
            log::info!("Auto-detected host platform");
            TargetPlatform::Host
        }
    }
    
    /// Create a driver for host development
    fn create_host_driver(app_title: &str, width: u32, height: u32) -> Result<Box<dyn PlatformDriver>> {
        // Try to create a rendering platform driver (uses SDL2)
        match rendering_driver::RenderingPlatformDriver::new(app_title, width, height) {
            Ok(driver) => {
                log::info!("Initialized rendering platform driver with SDL2 backend");
                Ok(Box::new(driver))
            },
            Err(e) => {
                // Log the error but provide a helpful message
                log::error!("Failed to initialize SDL2 rendering driver: {:?}", e);
                
                // Fallback to mock driver if SDL2 initialization failed
                log::warn!("Falling back to mock driver due to SDL2 initialization failure");
                let mock_driver = mock::MockDriver::new();
                Ok(Box::new(mock_driver))
            }
        }
    }
    
    /// Create a driver for embedded targets
    fn create_embedded_driver() -> Result<Box<dyn PlatformDriver>> {
        // TODO: In the future, implement proper framebuffer driver here
        // For now, use mock driver as a placeholder
        let mock_driver = mock::MockDriver::new();
        log::info!("Initialized mock platform driver for embedded target");
        
        Ok(Box::new(mock_driver))
    }
}