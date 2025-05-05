//! Event types for the PanelKit event system
//!
//! This module defines the event types used throughout the application.

use crate::primitives::Point;
use std::any::Any;
use std::fmt::Debug;

/// Base trait for all events
pub trait Event: Debug + Sync {
    /// Get the event type
    fn event_type(&self) -> EventType;
    
    /// Check if the event is handled
    fn is_handled(&self) -> bool;
    
    /// Mark the event as handled
    fn mark_handled(&mut self);
    
    /// Check if the event should be propagated
    fn should_propagate(&self) -> bool {
        !self.is_handled()
    }
    
    /// Convert to Any for downcasting
    fn as_any(&self) -> &dyn Any;
    
    /// Convert to Any for downcasting
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// Event phase in the propagation cycle
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EventPhase {
    /// Event is traveling down the component tree (parent to child)
    Capturing,
    
    /// Event is at its target component
    AtTarget,
    
    /// Event is traveling up the component tree (child to parent)
    Bubbling,
}

/// Generic event data
#[derive(Debug, Clone)]
pub struct EventData {
    /// Whether the event has been handled
    handled: bool,
    
    /// Current propagation phase
    phase: EventPhase,
    
    /// Time when the event was created
    timestamp: u64,
}

impl EventData {
    /// Create new event data
    pub fn new() -> Self {
        Self {
            handled: false,
            phase: EventPhase::Capturing,
            timestamp: std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_millis() as u64,
        }
    }
    
    /// Set the event phase
    pub fn set_phase(&mut self, phase: EventPhase) {
        self.phase = phase;
    }
    
    /// Get the current event phase
    pub fn phase(&self) -> EventPhase {
        self.phase
    }
    
    /// Get the event timestamp
    pub fn timestamp(&self) -> u64 {
        self.timestamp
    }
}

/// Type of event
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EventType {
    /// Touch-related events
    Touch,
    
    /// Keyboard-related events
    Keyboard,
    
    /// System events
    System,
    
    /// Custom application events
    Custom,
}

/// Touch action types
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum TouchAction {
    /// Touch started (finger down)
    Down,
    
    /// Touch moved while pressed
    Move,
    
    /// Touch ended (finger up)
    Up,
    
    /// Long press detected
    LongPress,
    
    /// Gesture detected
    Gesture(GestureType),
}

/// Gesture types
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum GestureType {
    /// Swipe gesture
    Swipe(SwipeDirection),
    
    /// Pinch gesture (zoom)
    Pinch { 
        /// Scale factor (>1 for zoom in, <1 for zoom out)
        scale: f32 
    },
    
    /// Rotation gesture
    Rotate { 
        /// Rotation angle in degrees
        angle: f32 
    },
}

/// Direction of a swipe gesture
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SwipeDirection {
    /// Left swipe
    Left,
    
    /// Right swipe
    Right,
    
    /// Up swipe
    Up,
    
    /// Down swipe
    Down,
}

/// Touch event information
#[derive(Debug, Clone)]
pub struct TouchEvent {
    /// Common event data
    pub data: EventData,
    
    /// Touch action
    pub action: TouchAction,
    
    /// Touch position
    pub position: Point,
    
    /// Previous touch position (for move events)
    pub previous_position: Option<Point>,
}

impl TouchEvent {
    /// Create a new touch event
    pub fn new(action: TouchAction, position: Point) -> Self {
        Self {
            data: EventData::new(),
            action,
            position,
            previous_position: None,
        }
    }
    
    /// Create a new touch move event
    pub fn new_move(position: Point, previous: Point) -> Self {
        Self {
            data: EventData::new(),
            action: TouchAction::Move,
            position,
            previous_position: Some(previous),
        }
    }
}

impl Event for TouchEvent {
    fn event_type(&self) -> EventType {
        EventType::Touch
    }
    
    fn is_handled(&self) -> bool {
        self.data.handled
    }
    
    fn mark_handled(&mut self) {
        self.data.handled = true;
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// Keyboard event information
#[derive(Debug, Clone)]
pub struct KeyboardEvent {
    /// Common event data
    pub data: EventData,
    
    /// Key code
    pub key_code: u32,
    
    /// Whether the key is pressed (true) or released (false)
    pub pressed: bool,
    
    /// Modifier keys that were active during this event
    pub modifiers: KeyModifiers,
}

/// Keyboard modifier state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct KeyModifiers {
    /// Shift key
    pub shift: bool,
    
    /// Ctrl key
    pub ctrl: bool,
    
    /// Alt key
    pub alt: bool,
    
    /// Meta/Super/Windows key
    pub meta: bool,
}

impl KeyboardEvent {
    /// Create a new keyboard event
    pub fn new(key_code: u32, pressed: bool) -> Self {
        Self {
            data: EventData::new(),
            key_code,
            pressed,
            modifiers: KeyModifiers {
                shift: false,
                ctrl: false,
                alt: false,
                meta: false,
            },
        }
    }
    
    /// Set modifier keys
    pub fn with_modifiers(mut self, modifiers: KeyModifiers) -> Self {
        self.modifiers = modifiers;
        self
    }
}

impl Event for KeyboardEvent {
    fn event_type(&self) -> EventType {
        EventType::Keyboard
    }
    
    fn is_handled(&self) -> bool {
        self.data.handled
    }
    
    fn mark_handled(&mut self) {
        self.data.handled = true;
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// System event information
#[derive(Debug, Clone)]
pub struct SystemEvent {
    /// Common event data
    pub data: EventData,
    
    /// Type of system event
    pub system_type: SystemEventType,
}

/// Types of system events
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SystemEventType {
    /// Window resized
    Resize {
        /// New width
        width: u32,
        
        /// New height
        height: u32,
    },
    
    /// Application is about to close
    AppClosing,
    
    /// Device orientation changed
    OrientationChange {
        /// Landscape or portrait
        landscape: bool,
    },
    
    /// System theme changed
    ThemeChange {
        /// Dark mode or not
        dark_mode: bool,
    },
}

impl SystemEvent {
    /// Create a new system event
    pub fn new(system_type: SystemEventType) -> Self {
        Self {
            data: EventData::new(),
            system_type,
        }
    }
}

impl Event for SystemEvent {
    fn event_type(&self) -> EventType {
        EventType::System
    }
    
    fn is_handled(&self) -> bool {
        self.data.handled
    }
    
    fn mark_handled(&mut self) {
        self.data.handled = true;
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// Custom application event
#[derive(Debug, Clone)]
pub struct CustomEvent {
    /// Common event data
    pub data: EventData,
    
    /// Custom event name
    pub name: String,
    
    /// Event payload as a string
    pub payload: String,
}

impl CustomEvent {
    /// Create a new custom event
    pub fn new(name: &str, payload: &str) -> Self {
        Self {
            data: EventData::new(),
            name: name.to_string(),
            payload: payload.to_string(),
        }
    }
}

impl Event for CustomEvent {
    fn event_type(&self) -> EventType {
        EventType::Custom
    }
    
    fn is_handled(&self) -> bool {
        self.data.handled
    }
    
    fn mark_handled(&mut self) {
        self.data.handled = true;
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}