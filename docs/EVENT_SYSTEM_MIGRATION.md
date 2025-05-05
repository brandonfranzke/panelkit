# Event System Migration Guide

This document provides guidance for migrating from the legacy event system (v0.2.x) to the new event system (v0.3.x).

## Overview of Changes

The PanelKit event system has been completely redesigned to provide:

1. **Type-safe events** - Using traits instead of enums
2. **Better event propagation** - With capturing, at-target, and bubbling phases
3. **Component-based event handling** - Direct integration with UI components
4. **Thread-safety improvements** - Events are Sync and Send

## Migration Timeline

- **v0.2.x**: Both systems available, legacy system marked as deprecated
- **v0.3.0**: Legacy system removed entirely

## Key Migration Steps

### 1. Update Event Types

**Legacy approach**:
```rust
use crate::event::Event;

fn handle_event(&mut self, event: &Event) {
    match event {
        Event::Touch { x, y, action } => {
            // Handle touch event...
        },
        Event::Key { key, pressed } => {
            // Handle key event...
        },
        // ...
    }
}
```

**New approach**:
```rust
use crate::event::{Event, TouchEvent, KeyboardEvent};

fn handle_new_event(&mut self, event: &mut dyn Event) {
    match event.event_type() {
        EventType::Touch => {
            if let Some(touch_event) = event.as_any_mut().downcast_mut::<TouchEvent>() {
                // Handle touch event with specific action...
                match touch_event.action {
                    TouchAction::Down => { /* ... */ },
                    TouchAction::Up => { /* ... */ },
                    TouchAction::Move => { /* ... */ },
                    // ...
                }
            }
        },
        EventType::Keyboard => {
            if let Some(key_event) = event.as_any_mut().downcast_mut::<KeyboardEvent>() {
                // Handle key event...
            }
        },
        // ...
    }
}
```

### 2. Update TouchAction Types

| Legacy TouchAction | New TouchAction   |
|--------------------|-------------------|
| `Press`            | `Down`            |
| `Release`          | `Up`              |
| `Move`             | `Move`            |
| `LongPress`        | `LongPress`       |
| `Swipe(dir)`       | `Gesture(GestureType::Swipe(dir))` |

### 3. Implement EventHandler for Components

For custom components, implement the `EventHandler` trait:

```rust
impl EventHandler for MyComponent {
    fn handle_event(&mut self, event: &mut dyn Event) -> Result<bool> {
        // Implement event handling...
        // Return true if handled, false otherwise
        Ok(false)
    }
}
```

Or by extending the Component trait which automatically implements EventHandler:

```rust
impl Component for MyComponent {
    // ... other Component methods ...
    
    // Implement specific event handlers:
    fn on_touch_down(&mut self, event: &mut TouchEvent) -> Result<bool> {
        // Handle touch down...
        Ok(true) // Return true if handled
    }
    
    fn on_touch_up(&mut self, event: &mut TouchEvent) -> Result<bool> {
        // Handle touch up...
        Ok(false) // Return false if not handled
    }
}
```

### 4. Replace EventBroker with EventBus

**Legacy approach**:
```rust
let event_broker = EventBroker::new();
let receiver = event_broker.subscribe("input");
event_broker.publish("input", Event::Custom { 
    event_type: "some_event".to_string(), 
    payload: "data".to_string() 
});
```

**New approach**:
```rust
let mut event_bus = EventBus::new();
let receiver = event_bus.subscribe("input");
event_bus.publish("input", CustomEvent::new("some_event", "data"));
```

### 5. Update Page Event Handling

For UI Pages, implement the `handle_new_event` method:

```rust
fn handle_new_event(&mut self, event: &mut dyn Event) -> Result<Option<String>> {
    match event.event_type() {
        EventType::Touch => {
            if let Some(touch_event) = event.as_any_mut().downcast_mut::<TouchEvent>() {
                if touch_event.action == TouchAction::Down {
                    // Check if a navigational element was clicked
                    if self.next_button.contains(&touch_event.position) {
                        touch_event.mark_handled(); // Mark as handled
                        return Ok(Some("next_page".to_string())); // Return navigation target
                    }
                }
            }
        },
        // Other event types...
        _ => {}
    }
    
    Ok(None) // No navigation
}
```

## Best Practices

1. **Mark events as handled** when appropriate using `event.mark_handled()`
2. **Check if events are already handled** using `event.is_handled()`
3. **Return true from handlers** when you've handled an event
4. **Implement Container trait** for components with children
5. **Use event bubbling phase** for event delegation

## Additional Resources

- See `src/ui/components/button.rs` for an example of Component event handling
- See `src/ui/hello_rendering_page.rs` for an example of Page event handling
- The `event::types` module contains all the new event types
- The `event::dispatch` module contains event dispatching mechanisms