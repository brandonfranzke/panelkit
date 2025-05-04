//! Platform abstraction for PanelKit
//!
//! This module provides platform-specific implementations for display, input, and timing.

use anyhow::Result;
use std::any::Any;

// Include platform implementations
pub mod mock;
pub mod sdl_driver;

/// Graphics context - a generic handle provided by platform drivers for UI rendering
pub trait GraphicsContext: Send + Sync {
    /// Get the context as Any for safe downcasting
    fn as_any(&self) -> &dyn Any;
    
    /// Get the context as mutable Any for safe downcasting
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// Unified platform driver interface that handles both display and input
pub trait PlatformDriver: Send {
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
        #[cfg(feature = "simulator")]
        {
            let sdl_driver = sdl_driver::SDLDriver::new(800, 480, "PanelKit Simulator")?;
            return Ok(Box::new(sdl_driver));
        }
        
        #[cfg(not(feature = "simulator"))]
        {
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

// Legacy traits for backward compatibility
// These are maintained for transition period and will be removed in the future

/// Display driver abstraction (legacy)
pub trait DisplayDriver: std::any::Any {
    /// Initialize the display
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    
    /// Flush rendering to the display
    fn flush(&mut self, buffer: &[u8]) -> Result<()>;
    
    /// Get display dimensions
    fn dimensions(&self) -> (u32, u32);
    
    /// Clean up resources
    fn cleanup(&mut self);
    
    /// Get the driver as Any for downcasting
    fn as_any(&self) -> &dyn std::any::Any;
    
    /// Get the driver as mutable Any for downcasting
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any;
}

/// Input driver abstraction (legacy)
pub trait InputDriver: std::any::Any {
    /// Initialize the input device
    fn init(&mut self) -> Result<()>;
    
    /// Poll for input events
    fn poll_events(&mut self) -> Result<Vec<crate::event::Event>>;
    
    /// Clean up resources
    fn cleanup(&mut self);
    
    /// Get the driver as Any for downcasting
    fn as_any(&self) -> &dyn std::any::Any;
    
    /// Get the driver as mutable Any for downcasting
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any;
}

/// Combined driver trait (legacy)
pub trait Driver {
    /// Initialize the display
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    
    /// Flush rendering to the display
    fn flush(&mut self, buffer: &[u8]) -> Result<()>;
    
    /// Get display dimensions
    fn dimensions(&self) -> (u32, u32);
    
    /// Initialize the input
    fn init_input(&mut self) -> Result<()>;
    
    /// Poll for input events
    fn poll_events(&mut self) -> Result<Vec<crate::event::Event>>;
    
    /// Clean up resources
    fn cleanup(&mut self);
    
    /// Get the driver as Any for downcasting
    fn as_any(&self) -> &dyn std::any::Any;
    
    /// Get the driver as mutable Any for downcasting
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any;
}

// Legacy factory methods - these will be removed once the transition is complete
impl PlatformFactory {
    /// Create a combined display and input driver (legacy)
    pub fn create_driver() -> Result<Box<dyn Driver>> {
        #[cfg(feature = "simulator")]
        {
            let sdl_driver = sdl_driver::SDLDriver::new(
                800, 480, "PanelKit Simulator"
            )?;
            return Ok(Box::new(sdl_driver));
        }
        
        #[cfg(not(feature = "simulator"))]
        {
            let mock_driver = mock::CombinedMockDriver::new();
            return Ok(Box::new(mock_driver));
        }
    }
    
    /// Create a display driver (legacy)
    pub fn create_display_driver() -> Box<dyn DisplayDriver> {
        Box::new(mock::MockDisplayDriver::new())
    }
    
    /// Create an input driver (legacy)
    pub fn create_input_driver() -> Box<dyn InputDriver> {
        Box::new(mock::MockInputDriver::new())
    }
}