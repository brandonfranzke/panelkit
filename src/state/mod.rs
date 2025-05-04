//! State management for PanelKit
//!
//! This module handles application state persistence and management.

use std::collections::HashMap;
use std::path::Path;
use std::sync::{Arc, Mutex};

/// Simplified state manager for proof-of-life
/// 
/// This version uses in-memory storage only for the initial testing phase.
/// A full implementation with proper persistence can be added later.
pub struct StateManager {
    memory_state: Arc<Mutex<HashMap<String, Vec<u8>>>>,
    _persistent_path: Option<String>, // Just store the path for now
}

impl StateManager {
    /// Create a new state manager with optional persistence path
    pub fn new(persistent_path: Option<&Path>) -> anyhow::Result<Self> {
        // For proof-of-life, just store the path string
        let path_str = persistent_path.map(|p| p.to_string_lossy().to_string());
        
        Ok(Self {
            memory_state: Arc::new(Mutex::new(HashMap::new())),
            _persistent_path: path_str,
        })
    }
    
    /// Set a value in the state store
    pub fn set<T: serde::Serialize>(&self, key: &str, value: &T, _persist: bool) -> crate::error::Result<()> {
        let serialized = bincode::serialize(value)
            .map_err(|e| crate::error::StateError::SerializationError(format!(
                "Failed to serialize value for key '{}': {}", key, e
            )))?;
        
        // Just update memory state for now
        self.memory_state.lock().unwrap().insert(key.to_string(), serialized);
        
        // Log the operation for proof-of-life
        log::debug!("State value set: {}", key);
        
        Ok(())
    }
    
    /// Get a value from the state store
    pub fn get<T: for<'de> serde::Deserialize<'de>>(&self, key: &str) -> crate::error::Result<Option<T>> {
        // Just check memory for now
        if let Some(data) = self.memory_state.lock().unwrap().get(key) {
            log::debug!("State value retrieved: {}", key);
            return Ok(Some(bincode::deserialize(data)
                .map_err(|e| crate::error::StateError::SerializationError(format!(
                    "Failed to deserialize value for key '{}': {}", key, e
                )))?));
        }
        
        log::debug!("State value not found: {}", key);
        Ok(None)
    }
    
    /// Load all persistent state into memory - stub implementation
    pub fn load_all(&self) -> crate::error::Result<()> {
        // Just log for proof-of-life
        log::info!("State manager initialized (persistence not implemented for proof-of-life)");
        Ok(())
    }
}