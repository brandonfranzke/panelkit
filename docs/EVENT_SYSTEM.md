# PanelKit Event System

This document describes the architecture and usage of the PanelKit event system, which provides type-safe event handling with proper propagation through the component hierarchy.

## Overview

The event system is based on Rust traits for type safety and flexibility:

**Trait-based event system**: Using Rust's trait system for type safety and downcasting

The architecture supports:

- **Type-safe event handling**: Through trait downcasting
- **Event propagation phases**: Capturing (parent to child), at-target, and bubbling (child to parent)
- **Event cloning**: For multi-subscriber scenarios
- **Event composition**: Events can be extended with additional data
- **Thread safety**: Events are Sync+Send for use across thread boundaries

## Core Components

### 1. Event Trait

The base trait that all events implement:

```rust
pub trait Event: Debug + Sync {
    /// Get the event type
    fn event_type(&self) -> EventType;
    
    /// Check if the event is handled
    fn is_handled(&self) -> bool;
    
    /// Mark the event as handled
    fn mark_handled(&mut self);
    
    /// Check if the event should be propagated
    fn should_propagate(&self) -> bool {
        !self.is_handled()
    }
    
    /// Convert to Any for downcasting
    fn as_any(&self) -> &dyn Any;
    
    /// Convert to Any for downcasting
    fn as_any_mut(&mut self) -> &mut dyn Any;
    
    /// Create a deep clone of the event
    fn clone_event(&self) -> Box<dyn Event + Send>;
}
```

### 2. Specific Event Types

```rust
/// Touch event information
pub struct TouchEvent {
    pub data: EventData,        // Common event data
    pub action: TouchAction,    // Touch action type
    pub position: Point,        // Touch position
    pub previous_position: Option<Point>, // For move events
}

/// Keyboard event information
pub struct KeyboardEvent {
    pub data: EventData,        // Common event data
    pub key_code: u32,          // Key code
    pub pressed: bool,          // Whether the key is pressed or released
    pub modifiers: KeyModifiers // Key modifiers (shift, ctrl, etc.)
}

/// System event information
pub struct SystemEvent {
    pub data: EventData,        // Common event data
    pub system_type: SystemEventType, // Type of system event
}

/// Custom application event
pub struct CustomEvent {
    pub data: EventData,        // Common event data
    pub name: String,           // Custom event name
    pub payload: String,        // Event payload as a string
}
```

### 3. EventBus

The event bus provides a pub/sub mechanism for event distribution:

```rust
pub struct EventBus {
    /// Sender channels mapped to topic names
    senders: HashMap<String, Vec<Sender<Box<dyn Event + Send>>>>,
}
```

### 4. EventHandler

The trait for components that can handle events:

```rust
pub trait EventHandler {
    /// Process an event
    fn handle_event(&mut self, event: &mut dyn Event) -> Result<bool>;
}
```

### 5. EventPropagator

Helper for creating an event propagation chain through a component hierarchy:

```rust
pub struct EventPropagator;

impl EventPropagator {
    /// Propagate an event through a component hierarchy
    pub fn propagate<E: Event + 'static>(
        event: &mut E,
        target: &mut dyn EventHandler,
        ancestors: &mut [&mut dyn EventHandler],
    ) -> Result<bool>;
}
```

## Event Propagation Phases

The event system implements standard DOM-like event phases:

1. **Capturing Phase**: Events travel from the root to the target component
2. **At-Target Phase**: Event is processed by the target component
3. **Bubbling Phase**: Event travels back up from the target to the root

```rust
/// Event phase in the propagation cycle
pub enum EventPhase {
    /// Event is traveling down the component tree (parent to child)
    Capturing,
    
    /// Event is at its target component
    AtTarget,
    
    /// Event is traveling up the component tree (child to parent)
    Bubbling,
}
```

## Event Dispatch Mechanism

The `EventDispatcher` provides type-based dispatch and handles event propagation:

```rust
pub struct EventDispatcher {
    /// Event listeners organized by event type ID
    listeners: HashMap<TypeId, Vec<Box<dyn Fn(&mut dyn Event) -> Result<()> + Send + Sync>>>,
}
```

## Box<dyn Event> Implementation

To allow boxed events to be used as events (critical for propagation through component hierarchies), the event trait is implemented for `Box<T>` where `T: Event + ?Sized`:

```rust
impl<T: Event + ?Sized> Event for Box<T> {
    fn event_type(&self) -> EventType {
        self.as_ref().event_type()
    }
    
    fn is_handled(&self) -> bool {
        self.as_ref().is_handled()
    }
    
    fn mark_handled(&mut self) {
        self.as_mut().mark_handled()
    }
    
    fn should_propagate(&self) -> bool {
        self.as_ref().should_propagate()
    }
    
    fn as_any(&self) -> &dyn Any {
        self.as_ref().as_any()
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self.as_mut().as_any_mut()
    }
    
    fn clone_event(&self) -> Box<dyn Event + Send> {
        self.as_ref().clone_event()
    }
}
```

## Using the Event System

### 1. Creating and Handling Events

```rust
// Creating a touch event
let touch_event = TouchEvent::new(
    TouchAction::Down,
    Point::new(100, 100)
);

// Handling events with downcasting
fn handle_event(&mut self, event: &mut dyn Event) -> Result<bool> {
    match event.event_type() {
        EventType::Touch => {
            if let Some(touch_event) = event.as_any_mut().downcast_mut::<TouchEvent>() {
                match touch_event.action {
                    TouchAction::Down => {
                        // Handle touch down
                        touch_event.mark_handled(); // Mark as handled to stop propagation
                        return Ok(true);
                    },
                    // Handle other actions...
                    _ => {}
                }
            }
        },
        // Handle other event types...
        _ => {}
    }
    
    Ok(false) // Not handled
}
```

### 2. Using the EventBus

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

// Handle received events
while let Ok(event) = receiver.try_recv() {
    // Process event
}
```

### 3. Using the EventDispatcher

```rust
// Create a new event dispatcher
let mut dispatcher = EventDispatcher::new();

// Register a listener for TouchEvent
dispatcher.add_listener::<TouchEvent, _>(|event: &mut TouchEvent| {
    // Handle touch event
    Ok(())
});

// Dispatch an event
let mut event = TouchEvent::new(TouchAction::Down, Point::new(100, 100));
dispatcher.dispatch(&mut event)?;
```

### 4. Event Propagation

```rust
// Create components with the EventHandler trait
let mut parent = ParentComponent::new();
let mut child = ChildComponent::new();

// Create an event
let mut event = TouchEvent::new(TouchAction::Down, Point::new(100, 100));

// Propagate the event
EventPropagator::propagate(
    &mut event,
    &mut child,
    &mut [&mut parent]
)?;
```


## Best Practices

1. **Handle events based on type**: Use `match event.event_type()` followed by downcasting
2. **Mark events as handled**: Use `event.mark_handled()` to stop propagation when appropriate
3. **Respect propagation phases**: Different actions may be appropriate in different phases
4. **Check if events are already handled**: Use `event.is_handled()` or `event.should_propagate()`
5. **Clone events when needed**: Use `event.clone_event()` for multi-subscriber scenarios

## Examples

The codebase includes several examples of components using the event system:

- `src/ui/components/button.rs`: Basic button component with event handling
- `src/ui/hello_rendering_page.rs`: Example page with touch event handling
- `src/ui/world_rendering_page.rs`: Another example page with event handling

## Testing Events

The event system is designed to be testable:

```rust
#[test]
fn test_touch_event_handling() {
    let mut component = TestComponent::new();
    
    // Create a touch event
    let mut event = TouchEvent::new(
        TouchAction::Down,
        Point::new(100, 100)
    );
    
    // Process the event
    let result = component.handle_event(&mut event);
    
    // Verify the result
    assert!(result.is_ok());
    assert!(event.is_handled());
}
```

## Future Enhancements

1. **Asynchronous event processing**: Adding async support for event handlers
2. **Event filtering**: More sophisticated event filtering mechanisms
3. **Memory optimization**: Pooling and recycling events for better performance
4. **Component-based focus management**: Improved keyboard navigation