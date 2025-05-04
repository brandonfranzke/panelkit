//! Event system for PanelKit
//!
//! This module implements a pub/sub event system for application-wide events.

use crossbeam_channel::{Receiver, Sender};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};

/// Represents an event in the system
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

/// Touch actions that can be performed
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TouchAction {
    Press,
    Release,
    Move,
    LongPress,
    Swipe(SwipeDirection),
}

/// Direction of a swipe gesture
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SwipeDirection {
    Left,
    Right,
    Up,
    Down,
}

/// Event broker that dispatches events to subscribers
pub struct EventBroker {
    subscribers: Arc<Mutex<HashMap<String, Vec<Sender<Event>>>>>,
}

impl EventBroker {
    /// Create a new event broker
    pub fn new() -> Self {
        Self {
            subscribers: Arc::new(Mutex::new(HashMap::new())),
        }
    }
    
    /// Subscribe to a specific topic
    pub fn subscribe(&self, topic: &str) -> Receiver<Event> {
        let (sender, receiver) = crossbeam_channel::unbounded();
        
        let mut subscribers = self.subscribers.lock().unwrap();
        let topic_subscribers = subscribers.entry(topic.to_string()).or_insert_with(Vec::new);
        topic_subscribers.push(sender);
        
        receiver
    }
    
    /// Publish an event to a specific topic
    pub fn publish(&self, topic: &str, event: Event) {
        let subscribers = self.subscribers.lock().unwrap();
        
        if let Some(topic_subscribers) = subscribers.get(topic) {
            for sender in topic_subscribers {
                let _ = sender.send(event.clone());
            }
        }
    }
}