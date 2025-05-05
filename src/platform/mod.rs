//! Platform abstraction for PanelKit
//!
//! This module provides platform-specific implementations for display, input, and timing.

use anyhow::Result;
use crate::TargetPlatform;
use crate::primitives::RenderingContext;
use crate::event::Event;

// Include platform implementations
pub mod mock;
pub mod sdl_driver;
pub mod rendering_driver;
pub mod detection;

/// Unified platform driver interface that handles both display and input
pub trait PlatformDriver {
    /// Initialize the driver with given dimensions
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    
    /// Poll for input events
    /// 
    /// Returns a vector of boxed Event trait objects
    fn poll_events(&mut self) -> Result<Vec<Box<dyn Event>>>;
    
    /// Present rendered content to the display
    fn present(&mut self) -> Result<()>;
    
    /// Get the display dimensions
    fn dimensions(&self) -> (u32, u32);
    
    /// Create a rendering context for drawing operations
    fn create_rendering_context(&mut self) -> Result<Box<dyn RenderingContext>>;
    
    /// Release all resources
    fn cleanup(&mut self);
}

/// Factory for creating platform-appropriate driver implementations
pub struct PlatformFactory;

impl PlatformFactory {
    /// Create a platform driver based on the specified target platform
    pub fn create(target_platform: TargetPlatform) -> Result<Box<dyn PlatformDriver>> {
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
                // Note: Initial dimensions will be set by the Application during init()
                Self::create_host_driver(app_title, 800, 480)
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
        detection::detect_platform()
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
    /// 
    /// # Future Improvements
    /// 
    /// This is currently a placeholder that uses a mock driver.
    /// In the future, this should be enhanced to:
    /// - Implement a proper framebuffer driver for embedded Linux
    /// - Support various touch input devices (using evdev or similar)
    /// - Auto-detect display capabilities and dimensions
    /// - Support hardware-accelerated rendering where available
    /// - Provide better error reporting for embedded devices
    fn create_embedded_driver() -> Result<Box<dyn PlatformDriver>> {
        // Currently using mock driver as placeholder until proper framebuffer
        // implementation is completed
        let mock_driver = mock::MockDriver::new();
        log::info!("Initialized mock platform driver for embedded target");
        
        Ok(Box::new(mock_driver))
    }
}