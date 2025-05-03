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
            let read_txn = db.begin_read()?;
            let table = read_txn.open_table(STATE_TABLE)?;
            
            // Following the compiler's suggestion, process data without temporaries
            let value_result = table.get(key);
            
            if let Ok(value_opt) = value_result {
                if let Some(data) = value_opt {
                    // Clone the data into a new vector that's not tied to the transaction
                    let bytes = data.value().to_vec();
                    
                    // Drop the transaction-related values early
                    drop(data);
                    drop(value_opt);
                    drop(table);
                    drop(read_txn);
                    
                    // Now deserialize from our cloned data
                    return Ok(Some(bincode::deserialize(&bytes)?));
                }
            }
        }
        
        Ok(None)
    }
    
    /// Load all persistent state into memory
    pub fn load_all(&self) -> anyhow::Result<()> {
        if let Some(db) = &self.persistent_db {
            let read_txn = db.begin_read()?;
            let table = read_txn.open_table(STATE_TABLE)?;
            
            // First collect all data into memory without holding references
            let mut collected_data = Vec::new();
            {
                let iter = table.iter()?;
                for item in iter {
                    let (key, value) = item?;
                    let key_str = key.value().to_string();
                    let value_bytes = value.value().to_vec();
                    collected_data.push((key_str, value_bytes));
                }
            }
            
            // Explicitly drop the table and transaction before using the collected data
            drop(table);
            drop(read_txn);
            
            // Now update our memory state with the collected data
            let mut memory_state = self.memory_state.lock().unwrap();
            for (key, value) in collected_data {
                memory_state.insert(key, value);
            }
        }
        
        Ok(())
    }
}