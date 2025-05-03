//! State management for PanelKit
//!
//! This module handles application state persistence and management.

use redb::{Database, ReadableTable, TableDefinition};
use std::path::Path;
use std::sync::{Arc, Mutex};

/// Key-value table definition for state storage
const STATE_TABLE: TableDefinition<&str, &[u8]> = TableDefinition::new("state");

/// Manages application state with persistence options
pub struct StateManager {
    memory_state: Arc<Mutex<std::collections::HashMap<String, Vec<u8>>>>,
    persistent_db: Option<Database>,
}

impl StateManager {
    /// Create a new state manager with optional persistence
    pub fn new(persistent_path: Option<&Path>) -> anyhow::Result<Self> {
        let persistent_db = if let Some(path) = persistent_path {
            Some(Database::create(path)?)
        } else {
            None
        };
        
        Ok(Self {
            memory_state: Arc::new(Mutex::new(std::collections::HashMap::new())),
            persistent_db,
        })
    }
    
    /// Set a value in the state store
    pub fn set<T: serde::Serialize>(&self, key: &str, value: &T, persist: bool) -> anyhow::Result<()> {
        let serialized = bincode::serialize(value)?;
        
        // Always update memory state
        self.memory_state.lock().unwrap().insert(key.to_string(), serialized.clone());
        
        // Maybe update persistent state
        if persist && self.persistent_db.is_some() {
            let db = self.persistent_db.as_ref().unwrap();
            let write_txn = db.begin_write()?;
            {
                let mut table = write_txn.open_table(STATE_TABLE)?;
                table.insert(key, serialized.as_slice())?;
            }
            write_txn.commit()?;
        }
        
        Ok(())
    }
    
    /// Get a value from the state store
    pub fn get<T: for<'de> serde::Deserialize<'de>>(&self, key: &str) -> anyhow::Result<Option<T>> {
        // First check memory
        if let Some(data) = self.memory_state.lock().unwrap().get(key) {
            return Ok(Some(bincode::deserialize(data)?));
        }
        
        // Then check persistent storage
        if let Some(db) = &self.persistent_db {
            // Create a block to ensure proper lifetime scoping
            let result = {
                let read_txn = db.begin_read()?;
                let table = read_txn.open_table(STATE_TABLE)?;
                
                if let Ok(value) = table.get(key) {
                    if let Some(data) = value {
                        // Clone the data to a Vec to avoid lifetime issues
                        let data_vec = data.value().to_vec();
                        Some(bincode::deserialize(&data_vec)?)
                    } else {
                        None
                    }
                } else {
                    None
                }
            };
            
            if result.is_some() {
                return Ok(result);
            }
        }
        
        Ok(None)
    }
    
    /// Load all persistent state into memory
    pub fn load_all(&self) -> anyhow::Result<()> {
        if let Some(db) = &self.persistent_db {
            // Create a scope to ensure proper lifetime handling
            {
                let read_txn = db.begin_read()?;
                let table = read_txn.open_table(STATE_TABLE)?;
                
                let mut memory_state = self.memory_state.lock().unwrap();
                let iter = table.iter()?;
                
                for item in iter {
                    let (key, value) = item?;
                    let key_str = key.value().to_string();
                    let value_bytes = value.value().to_vec();
                    memory_state.insert(key_str, value_bytes);
                }
                // The transaction will be dropped at the end of this scope
            }
        }
        
        Ok(())
    }
}