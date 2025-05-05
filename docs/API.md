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

PanelKit uses a trait-based event system that provides type safety and proper event propagation.

### Event Trait

The core of the event system is the `Event` trait:

```rust
pub trait Event: Debug + Sync {
    /// Get the event type
    fn event_type(&self) -> EventType;
    
    /// Check if the event is handled
    fn is_handled(&self) -> bool;
    
    /// Mark the event as handled
    fn mark_handled(&mut self);
    
    /// Convert to Any for downcasting
    fn as_any(&self) -> &dyn Any;
    
    /// Convert to Any for downcasting
    fn as_any_mut(&mut self) -> &mut dyn Any;
    
    /// Create a deep clone of the event
    fn clone_event(&self) -> Box<dyn Event + Send>;
}
```

### Specific Event Types

```rust
// Touch event
pub struct TouchEvent {
    pub data: EventData,        // Common event data
    pub action: TouchAction,    // Down, Up, Move, etc.
    pub position: Point,        // Touch position
}

// Keyboard event
pub struct KeyboardEvent {
    pub data: EventData,
    pub key_code: u32,
    pub pressed: bool,
}

// System event
pub struct SystemEvent {
    pub data: EventData,
    pub system_type: SystemEventType, // Resize, AppClosing, etc.
}

// Custom event
pub struct CustomEvent {
    pub data: EventData,
    pub name: String,
    pub payload: String,
}
```

### EventBus

```rust
// Create a new event bus
let mut event_bus = EventBus::new();

// Subscribe to a topic
let receiver = event_bus.subscribe("input");

// Publish an event
event_bus.publish("input", TouchEvent::new(
    TouchAction::Down,
    Point::new(100, 100)
));

// Process received events
while let Ok(mut event) = receiver.try_recv() {
    match event.event_type() {
        EventType::Touch => {
            if let Some(touch_event) = event.as_any_mut().downcast_mut::<TouchEvent>() {
                // Process touch event
            }
        },
        // Handle other event types
        _ => {}
    }
}
```

### Event Handler

```rust
// Implement the EventHandler trait
impl EventHandler for MyComponent {
    fn handle_event(&mut self, event: &mut dyn Event) -> Result<bool> {
        match event.event_type() {
            EventType::Touch => {
                if let Some(touch_event) = event.as_any_mut().downcast_mut::<TouchEvent>() {
                    if touch_event.action == TouchAction::Down {
                        // Handle touch down
                        touch_event.mark_handled(); // Stop propagation
                        return Ok(true);  // Return true if handled
                    }
                }
            },
            // Handle other event types
            _ => {}
        }
        
        Ok(false) // Not handled
    }
}
```

### Enum-based Events (Deprecated)

For backward compatibility, an enum-based event system is also provided:

```rust
// Deprecated: Use trait-based events instead
pub enum EnumEvent {
    Touch { x: i32, y: i32, action: EnumTouchAction },
    Key { key: String, pressed: bool },
    // Other event types...
}

// Convert between event systems
let trait_event = convert_enum_to_trait_event(&enum_event);
```

See `docs/EVENT_SYSTEM.md` for detailed documentation on the event system.

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

## Runtime Platform Selection

PanelKit uses runtime detection to select the appropriate platform implementation:

```rust
// In code, specify target platform:
let config = AppConfig {
    // ... other config ...
    target_platform: TargetPlatform::Host, // or TargetPlatform::Embedded or TargetPlatform::Auto
};

// Or use environment variables:
// PANELKIT_EMBEDDED=1 forces embedded mode
// PANELKIT_PLATFORM=host|embedded sets specific platform
```

The `Auto` option will detect the platform based on:
1. Architecture (ARM implies embedded)
2. Environment variables
3. Device files (checking for /dev/fb0)

This dynamic selection replaces the previous compile-time feature flags approach and allows a single binary to adapt to different environments.