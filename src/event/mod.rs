//! Event system for PanelKit
//!
//! This module provides a comprehensive event system with type-safe event handling,
//! propagation through component hierarchies, and pub/sub capabilities.

// Re-export old event types for backward compatibility
// TODO: Remove these in a future version once all code is migrated
mod legacy {
    use crate::event::types::SwipeDirection;
    
    #[deprecated(
        since = "0.2.0",
        note = "MIGRATION REQUIRED: Use the new Event trait and specific event types (TouchEvent, KeyboardEvent, etc.) from the types module instead. Will be removed in version 0.3.0."
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
        note = "MIGRATION REQUIRED: Use TouchAction from the types module instead. Map Press → Down, Release → Up. Will be removed in version 0.3.0."
    )]
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub enum TouchAction {
        Press,
        Release,
        Move,
        LongPress,
        Swipe(SwipeDirection),
        Up, // For compatibility with new action types
    }
}

pub mod types;
pub mod dispatch;

// Re-export new event types
pub use types::*;
pub use dispatch::*;

// Re-export legacy types for backward compatibility
pub use legacy::{Event as LegacyEvent, TouchAction as LegacyTouchAction};

// Export commonly used types at the module level
pub use types::{
    Event, TouchEvent, KeyboardEvent, SystemEvent, CustomEvent,
    EventType, TouchAction, GestureType, SwipeDirection, EventPhase
};

use crossbeam_channel::{Receiver, Sender, unbounded};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use anyhow::Result;
use crate::primitives::Point;

/// Converts legacy events to the new event system types
pub fn convert_legacy_event(event: &LegacyEvent) -> Box<dyn Event> {
    match event {
        LegacyEvent::Touch { x, y, action } => {
            let new_action = match action {
                legacy::TouchAction::Press => TouchAction::Down,
                legacy::TouchAction::Release => TouchAction::Up,
                legacy::TouchAction::Up => TouchAction::Up,
                legacy::TouchAction::Move => TouchAction::Move,
                legacy::TouchAction::LongPress => TouchAction::LongPress,
                legacy::TouchAction::Swipe(dir) => {
                    let new_dir = match dir {
                        SwipeDirection::Left => SwipeDirection::Left,
                        SwipeDirection::Right => SwipeDirection::Right,
                        SwipeDirection::Up => SwipeDirection::Up,
                        SwipeDirection::Down => SwipeDirection::Down,
                    };
                    TouchAction::Gesture(GestureType::Swipe(new_dir))
                }
            };
            
            Box::new(TouchEvent::new(new_action, Point::new(*x, *y)))
        },
        LegacyEvent::Key { key, pressed } => {
            // Convert string key to a simple hash-based key code for now
            let key_code = key.chars().fold(0, |acc, c| acc + c as u32);
            Box::new(KeyboardEvent::new(key_code, *pressed))
        },
        LegacyEvent::NetworkStatus(status) => {
            Box::new(CustomEvent::new("network_status", &status.to_string()))
        },
        LegacyEvent::StateChange { key, value } => {
            Box::new(CustomEvent::new(&format!("state_change.{}", key), value))
        },
        LegacyEvent::Navigate { page_id } => {
            Box::new(CustomEvent::new("navigate", page_id))
        },
        LegacyEvent::Custom { event_type, payload } => {
            Box::new(CustomEvent::new(event_type, payload))
        },
    }
}

/// Legacy event broker for backward compatibility
#[deprecated(
    since = "0.2.0",
    note = "MIGRATION REQUIRED: Use the EventBus from the dispatch module instead for type-safe event handling. Will be removed in version 0.3.0."
)]
pub struct EventBroker {
    subscribers: Arc<Mutex<HashMap<String, Vec<Sender<LegacyEvent>>>>>,
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
    pub fn subscribe(&self, topic: &str) -> Receiver<LegacyEvent> {
        let (sender, receiver) = unbounded();
        
        let mut subscribers = self.subscribers.lock().unwrap();
        let topic_subscribers = subscribers.entry(topic.to_string()).or_insert_with(Vec::new);
        topic_subscribers.push(sender);
        
        receiver
    }
    
    /// Publish an event to a specific topic
    pub fn publish(&self, topic: &str, event: LegacyEvent) {
        // First handle legacy subscribers
        let subscribers = self.subscribers.lock().unwrap();
        
        if let Some(topic_subscribers) = subscribers.get(topic) {
            for sender in topic_subscribers {
                let _ = sender.send(event.clone());
            }
        }
        
        // Also convert and publish to the new event system
        if let Ok(mut event_bus) = self.event_bus.lock() {
            let new_event = convert_legacy_event(&event);
            // Currently Box<dyn Event> doesn't implement Event itself
            // We'll need to implement this properly in the future
            // For now we'll skip the event_bus publish step
            // let _ = event_bus.publish(topic, new_event);
        }
    }
    
    /// Get access to the new event bus
    pub fn event_bus(&self) -> Result<dispatch::EventBus> {
        Ok(dispatch::EventBus::new())
    }
}