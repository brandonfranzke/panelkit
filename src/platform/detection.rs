//! Platform detection utilities
//!
//! This module provides utilities for detecting the current platform.

use crate::TargetPlatform;

/// Detect the current platform based on environment
///
/// This function determines whether the application is running on a host system (for development)
/// or an embedded system (for deployment) based on several heuristics:
///
/// 1. Architecture: ARM processors are likely embedded devices (Raspberry Pi, etc.)
/// 2. Environment variables: PANELKIT_EMBEDDED=1 forces embedded mode
/// 3. Device files: Checking for /dev/fb0 (framebuffer device, common on Linux embedded)
///
/// # Future Improvements
///
/// This detection could be enhanced in several ways:
/// - Better platform-specific detection for different embedded hardware
/// - Auto-detection of display dimensions from the target platform
/// - Support for more hardware-specific capabilities (touch controller type, etc.)
/// - Dynamic loading of appropriate drivers based on detected hardware
pub fn detect_platform() -> TargetPlatform {
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