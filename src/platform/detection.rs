//! Platform detection utilities
//!
//! This module provides utilities for detecting the current platform.

use crate::TargetPlatform;

/// Detect the current platform based on environment
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