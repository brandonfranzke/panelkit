# PanelKit API Reference

This document provides an overview of PanelKit's core public APIs.

## Application

The `Application` struct is the main entry point to PanelKit.

```rust
// Create a new application instance
let config = AppConfig { /* ... */ };
let mut app = Application::new(config)?;

// Initialize and run the application
app.init()?;
app.run()?;

// Clean up resources
app.cleanup();
```

### AppConfig

```rust
struct AppConfig {
    width: u32,          // Display width in pixels
    height: u32,         // Display height in pixels
    fullscreen: bool,    // Whether to run in fullscreen mode
    state_path: Option<std::path::PathBuf>, // Path to state database
    log_level: log::LevelFilter, // Logging verbosity
}
```

## UI System

### Page Trait

Implement this trait to create custom UI pages:

```rust
pub trait Page {
    fn init(&mut self) -> Result<()>;
    fn render(&self) -> Result<()>;
    fn handle_event(&mut self, event: &Event) -> Result<()>;
    fn on_activate(&mut self) -> Result<()>;
    fn on_deactivate(&mut self) -> Result<()>;
    fn as_any(&self) -> &dyn Any;
    fn as_any_mut(&mut self) -> &mut dyn Any;
}
```

Example implementation:

```rust
struct MyPage {
    // Page state
}

impl Page for MyPage {
    fn init(&mut self) -> Result<()> {
        // Initialize page
        Ok(())
    }
    
    fn render(&self) -> Result<()> {
        // Render UI elements
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<()> {
        // Process input events
        match event {
            Event::Touch { x, y, action } => {
                // Handle touch events
            },
            _ => {}
        }
        Ok(())
    }
    
    fn on_activate(&mut self) -> Result<()> {
        // Page becoming active
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        // Page becoming inactive
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any { self }
    fn as_any_mut(&mut self) -> &mut dyn Any { self }
}
```

## Platform Abstraction

### PlatformDriver Trait

This trait defines the interface for platform-specific implementations:

```rust
pub trait PlatformDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    fn poll_events(&mut self) -> Result<Vec<Event>>;
    fn present(&mut self) -> Result<()>;
    fn dimensions(&self) -> (u32, u32);
    fn graphics_context(&self) -> Option<&dyn GraphicsContext>;
    fn cleanup(&mut self);
}
```

### GraphicsContext Trait

Provides access to platform-specific rendering capabilities:

```rust
pub trait GraphicsContext {
    fn as_any(&self) -> &dyn Any;
    fn as_any_mut(&mut self) -> &mut dyn Any;
}
```

Note: The platform abstraction is designed for a single-threaded application model in the current implementation. Thread-safety constraints have been removed to simplify SDL2 integration.

## Event System

### Event Types

```rust
pub enum Event {
    Touch {
        x: i32,
        y: i32,
        action: TouchAction,
    },
    NetworkStatus(bool),
    StateChange {
        key: String,
        value: String,
    },
    Custom {
        event_type: String,
        payload: String,
    },
}

pub enum TouchAction {
    Press,
    Release,
    Move,
    LongPress,
    Swipe(SwipeDirection),
}

pub enum SwipeDirection {
    Left,
    Right,
    Up,
    Down,
}
```

### Event Broker

```rust
// Create and use the event broker
let broker = event::EventBroker::new();

// Subscribe to events
let receiver = broker.subscribe("input");

// Publish events
broker.publish("input", Event::Touch { /* ... */ });

// Process events
while let Ok(event) = receiver.try_recv() {
    // Handle event
}
```

## State Management

```rust
// Create state manager
let state_manager = state::StateManager::new(Some(path))?;

// Store values (note: persistence flag is currently ignored)
state_manager.set("setting_name", &value, true)?;

// Retrieve values
if let Some(value) = state_manager.get::<String>("setting_name")? {
    // Use value
}
```

Note: The current implementation only supports in-memory storage. Persistence to disk is not yet implemented in this proof-of-concept but the architecture is designed to support it in the future.

## Error Handling

PanelKit uses `anyhow` for error handling:

```rust
// Propagate errors with context
fn do_something() -> Result<()> {
    operation_that_might_fail()
        .context("Failed during specific operation")?;
    Ok(())
}

// Handle errors gracefully
match risky_operation() {
    Ok(result) => {
        // Use result
    },
    Err(e) => {
        log::error!("Operation failed: {:#}", e);
        // Handle error or provide fallback
    }
}
```

## Feature Flags

Configure PanelKit behavior with Cargo features:

```toml
# Cargo.toml

[features]
default = ["simulator"]
simulator = ["sdl2"]
target = []
```

These flags control which platform implementation is used at runtime.