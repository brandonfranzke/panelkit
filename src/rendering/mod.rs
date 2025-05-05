//! Rendering abstraction layer for PanelKit
//!
//! This module provides a clean abstraction over different rendering backends,
//! allowing the UI to be independent of the underlying graphics implementation.

use anyhow::Result;
use crate::TargetPlatform;
use crate::primitives::RenderingContext;

pub mod sdl_backend;
pub mod fb_backend;

// We now use RenderingContext and Surface from primitives.rs

/// Factory for creating rendering contexts based on environment
pub struct RenderingFactory;

impl RenderingFactory {
    /// Create a new rendering context appropriate for the current environment
    pub fn create(title: &str) -> Result<Box<dyn RenderingContext>> {
        Self::create_for_platform(title, TargetPlatform::Auto)
    }
    
    /// Create a new rendering context for a specific platform
    pub fn create_for_platform(title: &str, platform: TargetPlatform) -> Result<Box<dyn RenderingContext>> {
        // Resolve auto platform
        let target = if platform == TargetPlatform::Auto {
            Self::detect_platform()
        } else {
            platform
        };
        
        log::info!("Creating rendering context for target: {:?}", target);
        
        match target {
            TargetPlatform::Host => Self::create_sdl_backend(title),
            TargetPlatform::Embedded => Self::create_fb_backend(),
            TargetPlatform::Auto => unreachable!("Auto platform should have been resolved"),
        }
    }
    
    /// Detect the current platform
    fn detect_platform() -> TargetPlatform {
        // Use the shared platform detection function
        crate::platform::detection::detect_platform()
    }
    
    /// Create an SDL backend for host systems
    fn create_sdl_backend(title: &str) -> Result<Box<dyn RenderingContext>> {
        // Try to create an SDL backend
        match sdl_backend::SDLBackend::new(title) {
            Ok(backend) => {
                log::info!("Created SDL rendering context");
                Ok(Box::new(backend))
            },
            Err(e) => {
                // Log failure and fallback to framebuffer
                log::error!("Failed to create SDL backend: {:?}", e);
                log::warn!("Falling back to framebuffer backend");
                
                Self::create_fb_backend()
            }
        }
    }
    
    /// Create a framebuffer backend for embedded systems
    fn create_fb_backend() -> Result<Box<dyn RenderingContext>> {
        match fb_backend::FramebufferBackend::new() {
            Ok(backend) => {
                log::info!("Created framebuffer rendering context");
                Ok(Box::new(backend))
            },
            Err(e) => {
                log::error!("Failed to create framebuffer backend: {:?}", e);
                anyhow::bail!("No available rendering backend")
            }
        }
    }
}