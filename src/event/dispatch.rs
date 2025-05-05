//! Event dispatch system for PanelKit
//!
//! This module provides a type-safe event dispatch system with proper 
//! propagation through the component hierarchy.

use super::types::*;
use anyhow::Result;
use std::any::TypeId;
use std::collections::HashMap;
use crossbeam_channel::{Sender, Receiver, unbounded};

/// Event dispatcher that can route events to registered listeners
pub struct EventDispatcher {
    /// Event listeners organized by event type ID
    listeners: HashMap<TypeId, Vec<Box<dyn Fn(&mut dyn Event) -> Result<()> + Send + Sync>>>,
}

impl EventDispatcher {
    /// Create a new event dispatcher
    pub fn new() -> Self {
        Self {
            listeners: HashMap::new(),
        }
    }
    
    /// Register a listener for a specific event type
    pub fn add_listener<E, F>(&mut self, listener: F) 
    where
        E: Event + 'static,
        F: Fn(&mut E) -> Result<()> + Send + Sync + 'static,
    {
        let type_id = TypeId::of::<E>();
        let wrapper = move |event: &mut dyn Event| -> Result<()> {
            // Downcast to the specific event type
            if let Some(e) = event.as_any_mut().downcast_mut::<E>() {
                listener(e)
            } else {
                Ok(()) // Not the right type, do nothing
            }
        };
        
        let listeners = self.listeners.entry(type_id).or_insert_with(Vec::new);
        listeners.push(Box::new(wrapper));
    }
    
    /// Dispatch an event to all registered listeners
    pub fn dispatch<E: Event + 'static>(&self, event: &mut E) -> Result<()> {
        let type_id = TypeId::of::<E>();
        
        if let Some(listeners) = self.listeners.get(&type_id) {
            for listener in listeners {
                listener(event)?;
                
                // Stop propagation if the event was handled
                if event.is_handled() {
                    break;
                }
            }
        }
        
        Ok(())
    }
}

/// A channel-based event bus for passing events between components
pub struct EventBus {
    /// Sender channels mapped to topic names
    senders: HashMap<String, Vec<Sender<Box<dyn Event + Send>>>>,
}

impl EventBus {
    /// Create a new event bus
    pub fn new() -> Self {
        Self {
            senders: HashMap::new(),
        }
    }
    
    /// Subscribe to events on a specific topic
    pub fn subscribe(&mut self, topic: &str) -> Receiver<Box<dyn Event + Send>> {
        let (sender, receiver) = unbounded();
        
        let topic_senders = self.senders.entry(topic.to_string()).or_insert_with(Vec::new);
        topic_senders.push(sender);
        
        receiver
    }
    
    /// Publish an event to a specific topic
    pub fn publish<E: Event + Send + Clone + 'static>(&self, topic: &str, event: E) -> Result<()> {
        if let Some(senders) = self.senders.get(topic) {
            if senders.is_empty() {
                return Ok(());
            }
            
            // For the case where we only have one subscriber, do a direct send
            if senders.len() == 1 {
                return senders[0].send(Box::new(event)).map_err(|e| anyhow::anyhow!("{}", e));
            }
            
            // Create clones for all subscribers but the last one
            for (i, sender) in senders.iter().enumerate() {
                if i < senders.len() - 1 {
                    // Clone for all but the last subscriber
                    let cloned_event = event.clone();
                    sender.send(Box::new(cloned_event))?;
                }
            }
            
            // Send the original event to the last subscriber
            let last_sender = &senders[senders.len() - 1];
            last_sender.send(Box::new(event))?;
        }
        
        Ok(())
    }
    
    /// Publish an already boxed event to a specific topic
    pub fn publish_boxed(&self, topic: &str, event: Box<dyn Event + Send>) -> Result<()> {
        if let Some(senders) = self.senders.get(topic) {
            if senders.is_empty() {
                return Ok(());
            }
            
            // For the case where we only have one subscriber, do a direct send
            if senders.len() == 1 {
                return senders[0].send(event).map_err(|e| anyhow::anyhow!("{}", e));
            }
            
            // Otherwise, handle multiple subscribers by sending clones to all but the last one
            for (i, sender) in senders.iter().enumerate() {
                if i < senders.len() - 1 {
                    // Clone for all but the last subscriber
                    let cloned_event = event.clone_event();
                    sender.send(cloned_event)?;
                }
            }
            
            // Send the original event to the last subscriber
            let last_sender = &senders[senders.len() - 1];
            last_sender.send(event)?;
        }
        
        Ok(())
    }
}

/// Trait for objects that can handle events
pub trait EventHandler {
    /// Process an event
    fn handle_event(&mut self, event: &mut dyn Event) -> Result<bool>;
}

/// Helper for creating an event dispatch chain through a component hierarchy
pub struct EventPropagator;

impl EventPropagator {
    /// Propagate an event through a component hierarchy
    pub fn propagate<E: Event + 'static>(
        event: &mut E,
        target: &mut dyn EventHandler,
        ancestors: &mut [&mut dyn EventHandler],
    ) -> Result<bool> {
        // First propagate down (capturing phase)
        event.as_any_mut().downcast_mut::<EventData>()
            .map(|data| data.set_phase(EventPhase::Capturing));
        
        // Go through ancestors from root to target (excluding target)
        for handler in ancestors.iter_mut() {
            let handled = handler.handle_event(event)?;
            if handled || event.is_handled() {
                return Ok(true);
            }
        }
        
        // Now at target
        event.as_any_mut().downcast_mut::<EventData>()
            .map(|data| data.set_phase(EventPhase::AtTarget));
        
        let handled = target.handle_event(event)?;
        if handled || event.is_handled() {
            return Ok(true);
        }
        
        // Finally bubble up (if applicable)
        event.as_any_mut().downcast_mut::<EventData>()
            .map(|data| data.set_phase(EventPhase::Bubbling));
        
        // Go through ancestors from target to root
        for handler in ancestors.iter_mut().rev() {
            let handled = handler.handle_event(event)?;
            if handled || event.is_handled() {
                return Ok(true);
            }
        }
        
        Ok(false)
    }
}