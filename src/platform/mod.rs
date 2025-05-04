//! Platform abstraction for PanelKit
//!
//! This module provides platform-specific implementations for display, input, and timing.

use anyhow::Result;
use std::any::Any;

// Include platform implementations
pub mod mock;
pub mod sdl_driver;

/// Graphics context - a generic handle provided by platform drivers for UI rendering
pub trait GraphicsContext {
    /// Get the context as Any for safe downcasting
    fn as_any(&self) -> &dyn Any;
    
    /// Get the context as mutable Any for safe downcasting
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

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
    
    /// Get a graphics context for rendering
    fn graphics_context(&self) -> Option<&dyn GraphicsContext>;
    
    /// Release all resources
    fn cleanup(&mut self);
}

/// Factory for creating platform-appropriate driver implementations
pub struct PlatformFactory;

impl PlatformFactory {
    /// Create a platform driver based on the current environment
    pub fn create() -> Result<Box<dyn PlatformDriver>> {
        log::info!("PlatformFactory: Creating platform driver...");
        
        #[cfg(feature = "simulator")]
        {
            log::info!("PlatformFactory: 'simulator' feature enabled, creating SDL driver");
            let sdl_driver = sdl_driver::SDLDriver::new(800, 480, "PanelKit Simulator")?;
            log::info!("PlatformFactory: SDL driver created successfully");
            return Ok(Box::new(sdl_driver));
        }
        
        #[cfg(not(feature = "simulator"))]
        {
            log::info!("PlatformFactory: 'simulator' feature disabled, creating mock driver");
            let mock_driver = mock::MockDriver::new();
            log::info!("PlatformFactory: Mock driver created successfully");
            return Ok(Box::new(mock_driver));
        }
        
        // This should be unreachable due to the cfg blocks above
        #[allow(unreachable_code)]
        {
            log::warn!("PlatformFactory: Reached unreachable code path - using mock driver as fallback");
            let mock_driver = mock::MockDriver::new();
            Ok(Box::new(mock_driver))
        }
    }
}

// No legacy traits needed - this is a clean implementation