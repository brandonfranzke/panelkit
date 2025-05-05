//! Event system for PanelKit
//!
//! This module provides a comprehensive event system with type-safe event handling,
//! propagation through component hierarchies, and pub/sub capabilities.

// Enum-based event representation
// Provides a simpler, centralized event hierarchy
mod enum_events {
    use crate::event::types::SwipeDirection;
    
    #[deprecated(
        since = "0.2.0",
        note = "Use the trait-based Event interface and specific event types (TouchEvent, KeyboardEvent, etc.) from the types module for better type safety and event propagation."
    )]
    #[derive(Debug, Clone)]
    pub enum Event {
        /// Touch event with coordinates and action
        Touch {
            x: i32,
            y: i32,
            action: TouchAction,
        },
        
        /// Key event with key name and press/release state
        Key {
            key: String,
            pressed: bool,
        },
        
        /// Network status change
        NetworkStatus(bool),
        
        /// Application state change
        StateChange {
            key: String,
            value: String,
        },
        
        /// Navigation event to a specific page
        Navigate {
            page_id: String,
        },
        
        /// Custom event with a type and payload
        Custom {
            event_type: String,
            payload: String,
        },
    }

    #[deprecated(
        since = "0.2.0",
        note = "Use the trait-based TouchAction from the types module instead for consistency with trait-based events."
    )]
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub enum TouchAction {
        Press,     // Touch down event
        Release,   // Touch release (corresponds to Up in trait-based)
        Move,      // Touch movement
        LongPress, // Long press gesture
        Swipe(SwipeDirection), // Swipe gesture with direction
        Up,        // Alternative to Release
    }
}

pub mod types;
pub mod dispatch;

// Re-export trait-based event types
pub use types::*;
pub use dispatch::*;

// Re-export enum-based types for code that uses the enum approach
pub use enum_events::{Event as EnumEvent, TouchAction as EnumTouchAction};

// Export commonly used trait-based types at the module level
pub use types::{
    Event, TouchEvent, KeyboardEvent, SystemEvent, CustomEvent,
    EventType, TouchAction, GestureType, SwipeDirection, EventPhase
};

use crossbeam_channel::{Receiver, Sender, unbounded};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use anyhow::Result;
use crate::primitives::Point;

/// Converts enum-based events to trait-based event types
pub fn convert_enum_to_trait_event(event: &EnumEvent) -> Box<dyn Event> {
    match event {
        EnumEvent::Touch { x, y, action } => {
            let trait_action = match action {
                enum_events::TouchAction::Press => TouchAction::Down,
                enum_events::TouchAction::Release => TouchAction::Up,
                enum_events::TouchAction::Up => TouchAction::Up,
                enum_events::TouchAction::Move => TouchAction::Move,
                enum_events::TouchAction::LongPress => TouchAction::LongPress,
                enum_events::TouchAction::Swipe(dir) => {
                    let trait_dir = match dir {
                        SwipeDirection::Left => SwipeDirection::Left,
                        SwipeDirection::Right => SwipeDirection::Right,
                        SwipeDirection::Up => SwipeDirection::Up,
                        SwipeDirection::Down => SwipeDirection::Down,
                    };
                    TouchAction::Gesture(GestureType::Swipe(trait_dir))
                }
            };
            
            Box::new(TouchEvent::new(trait_action, Point::new(*x, *y)))
        },
        EnumEvent::Key { key, pressed } => {
            // Convert string key to a simple hash-based key code for now
            let key_code = key.chars().fold(0, |acc, c| acc + c as u32);
            Box::new(KeyboardEvent::new(key_code, *pressed))
        },
        EnumEvent::NetworkStatus(status) => {
            Box::new(CustomEvent::new("network_status", &status.to_string()))
        },
        EnumEvent::StateChange { key, value } => {
            Box::new(CustomEvent::new(&format!("state_change.{}", key), value))
        },
        EnumEvent::Navigate { page_id } => {
            Box::new(CustomEvent::new("navigate", page_id))
        },
        EnumEvent::Custom { event_type, payload } => {
            Box::new(CustomEvent::new(event_type, payload))
        },
    }
}

/// Event broker using enum-based events
#[deprecated(
    since = "0.2.0",
    note = "Use the EventBus from the dispatch module instead for type-safe event handling. Will be removed in version 0.3.0."
)]
pub struct EventBroker {
    subscribers: Arc<Mutex<HashMap<String, Vec<Sender<EnumEvent>>>>>,
    event_bus: Arc<Mutex<dispatch::EventBus>>,
}

impl EventBroker {
    /// Create a new event broker
    pub fn new() -> Self {
        Self {
            subscribers: Arc::new(Mutex::new(HashMap::new())),
            event_bus: Arc::new(Mutex::new(dispatch::EventBus::new())),
        }
    }
    
    /// Subscribe to a specific topic
    pub fn subscribe(&self, topic: &str) -> Receiver<EnumEvent> {
        let (sender, receiver) = unbounded();
        
        let mut subscribers = self.subscribers.lock().unwrap();
        let topic_subscribers = subscribers.entry(topic.to_string()).or_insert_with(Vec::new);
        topic_subscribers.push(sender);
        
        receiver
    }
    
    /// Publish an event to a specific topic
    pub fn publish(&self, topic: &str, event: EnumEvent) {
        // First handle enum-based event subscribers
        let subscribers = self.subscribers.lock().unwrap();
        
        if let Some(topic_subscribers) = subscribers.get(topic) {
            for sender in topic_subscribers {
                let _ = sender.send(event.clone());
            }
        }
        
        // Also convert and publish to the trait-based event system
        if let Ok(mut event_bus) = self.event_bus.lock() {
            let trait_event = convert_enum_to_trait_event(&event);
            // Use the publish_boxed method that accepts Box<dyn Event> directly
            let _ = event_bus.publish_boxed(topic, trait_event);
        }
    }
    
    /// Get access to the trait-based event bus
    pub fn event_bus(&self) -> Result<dispatch::EventBus> {
        Ok(dispatch::EventBus::new())
    }
}