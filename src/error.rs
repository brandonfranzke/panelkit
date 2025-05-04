//! Error handling for PanelKit
//!
//! This module provides consistent error types and handling for the application.

use thiserror::Error;
use std::fmt;

// Re-export anyhow types for convenience
pub use anyhow::{Context, Result as AnyhowResult};

/// Main error type for PanelKit
#[derive(Error, Debug)]
pub enum Error {
    /// Platform-specific errors
    #[error("Platform error: {0}")]
    Platform(#[from] PlatformError),
    
    /// UI-related errors
    #[error("UI error: {0}")]
    UI(#[from] UIError),
    
    /// State management errors
    #[error("State error: {0}")]
    State(#[from] StateError),
    
    /// I/O errors
    #[error("I/O error: {0}")]
    IO(#[from] std::io::Error),
    
    /// Serialization/deserialization errors
    #[error("Serialization error: {0}")]
    Serialization(String),
    
    /// Configuration errors
    #[error("Configuration error: {0}")]
    Configuration(String),
    
    /// Generic errors with context
    #[error("{context}: {source}")]
    WithContext {
        context: String,
        source: Box<dyn std::error::Error + Send + Sync>,
    },
    
    /// Wrap anyhow error
    #[error("Error: {0}")]
    Anyhow(#[from] anyhow::Error),
}

impl Error {
    /// Create a new error with context
    pub fn with_context<E, S>(error: E, context: S) -> Self 
    where 
        E: std::error::Error + Send + Sync + 'static,
        S: Into<String>,
    {
        Error::WithContext {
            context: context.into(),
            source: Box::new(error),
        }
    }
}

/// Platform-specific errors
#[derive(Error, Debug)]
pub enum PlatformError {
    /// SDL-specific errors
    #[error("SDL error: {0}")]
    SDL(String),
    
    /// Graphics context errors
    #[error("Graphics context error: {0}")]
    GraphicsContext(String),
    
    /// Input handling errors
    #[error("Input error: {0}")]
    Input(String),
    
    /// Initialization errors
    #[error("Initialization error: {0}")]
    Initialization(String),
}

/// UI-related errors
#[derive(Error, Debug)]
pub enum UIError {
    /// Page not found
    #[error("Page not found: {0}")]
    PageNotFound(String),
    
    /// Page rendering error
    #[error("Failed to render page: {0}")]
    RenderingError(String),
    
    /// Navigation error
    #[error("Navigation error: {0}")]
    NavigationError(String),
    
    /// Component error
    #[error("Component error: {0}")]
    ComponentError(String),
}

/// State management errors
#[derive(Error, Debug)]
pub enum StateError {
    /// Key not found
    #[error("Key not found: {0}")]
    KeyNotFound(String),
    
    /// Serialization error
    #[error("Serialization error: {0}")]
    SerializationError(String),
    
    /// Persistence error
    #[error("Persistence error: {0}")]
    PersistenceError(String),
}

/// Create a result type alias with our custom error
pub type Result<T> = std::result::Result<T, Error>;