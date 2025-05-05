//! Event system for PanelKit
//!
//! This module provides a comprehensive event system with type-safe event handling,
//! propagation through component hierarchies, and pub/sub capabilities.

pub mod types;
pub mod dispatch;

// Export all event types
pub use types::*;
pub use dispatch::*;

// Export commonly used types at the module level for convenience
pub use types::{
    Event, TouchEvent, KeyboardEvent, SystemEvent, CustomEvent,
    EventType, TouchAction, GestureType, SwipeDirection, EventPhase
};

