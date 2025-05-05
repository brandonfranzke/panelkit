# PanelKit Architecture

This document describes the core architecture of the PanelKit embedded UI system.

## System Overview

PanelKit is a lightweight UI application designed for embedded Linux devices with touchscreens. It operates directly on the framebuffer or through SDL2 for development, without requiring a window manager or desktop environment.

The architecture follows a modular design with clean separation of concerns:

```
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│  Application  │     │  UI Manager   │     │  Pages        │
│  - Core loop  │◄───►│  - Navigation │◄───►│  - Content    │
│  - Lifecycle  │     │  - Layout     │     │  - Interaction│
└───────┬───────┘     └───────┬───────┘     └───────────────┘
        │                     │
        ▼                     ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│ Event System  │     │Platform Driver │     │ State Manager │
│ - Pub/sub     │◄───►│ - Display     │◄───►│ - Data store  │
│ - Dispatching │     │ - Input       │     │ - Persistence │
└───────────────┘     └───────────────┘     └───────────────┘
```

## Core Components

### 1. Application (lib.rs)

The central component that orchestrates the entire system:
- Initializes all subsystems 
- Manages the application lifecycle
- Runs the main event loop
- Connects the UI with platform drivers

### 2. UI System (ui/mod.rs)

Manages the user interface:
- **UIManager**: Handles page navigation and layout
- **Page trait**: Interface for all UI pages
- **Safely typed**: Uses downcasting (`as_any/as_any_mut`) for type-safe access to concrete types
- **Direct rendering**: Currently uses SDL2 Canvas directly (rather than a declarative approach)
- **LVGL integration**: Originally planned but deferred due to integration challenges (see ARCHITECTURE_DECISIONS.md)

### 3. Platform Abstraction (platform/mod.rs)

Provides unified hardware abstraction:
- **PlatformDriver**: Core unified interface for all platforms
- **RenderingContext**: Abstract rendering interface for all graphics operations
- **Implementation variants**:
  - RenderingPlatformDriver: Primary implementation using rendering backends
  - SDLBackend: For host development using SDL2
  - FramebufferBackend: For embedded targets using framebuffer
  - MockDriver: For testing and headless operation

The platform abstraction layer uses runtime polymorphism to select the appropriate implementation based on the detected platform or explicit configuration.

### 4. Event System (event/mod.rs, event/types.rs, event/dispatch.rs)

Implements a robust event system:
- **Trait-based events**: Using Rust's trait system with downcasting for type safety
- **Event propagation phases**: Capturing (parent to child), at-target, and bubbling (child to parent)
- **EventBus**: Type-safe pub/sub message dispatcher with multi-subscriber support
- **EventHandler trait**: Components implement this for event processing
- **EventDispatcher**: Efficient routing of events to appropriate handlers
- **Event cloning mechanism**: Box<dyn Event>::clone_event() for multi-subscriber scenarios
- **Box<dyn Event> support**: Full Event trait implementation for boxed trait objects
- **Thread-safe events**: Sync+Send bounds for concurrent processing

### 5. State Management (state/mod.rs)

Manages application state:
- **In-memory state storage** (current implementation)
- **Serialization** of configuration and state data
- **State persistence** (architecture prepared, not yet implemented)
- **Reactive state** (planned for future implementation)

## Key Design Patterns

1. **Trait-based abstraction**: All platform-specific code is accessed through traits
2. **Type-safe downcasting**: Components use `as_any` pattern for safe concrete type access
3. **Publisher/subscriber**: Events are dispatched through a central broker
4. **Factory pattern**: PlatformFactory creates appropriate driver implementations
5. **Error propagation**: Contextual error handling with anyhow

## Cross-Platform Strategy

PanelKit achieves cross-platform compatibility through multiple mechanisms:

1. **Runtime polymorphism**: Platform implementations are dynamically selected at runtime based on environment detection
2. **Unified rendering abstraction**: All rendering operations go through the `RenderingContext` interface
3. **Graceful fallbacks**: System degrades gracefully when preferred backends are unavailable
4. **Type-safe downcasting**: Backend-specific features are accessible through safe downcasting when needed
5. **Common event model**: Input from different sources maps to a unified event type

## Error Handling

The application uses a comprehensive error handling strategy:
- **Contextual errors**: Each error includes its source context for easier debugging
- **Graceful degradation**: Main loop continues despite non-critical errors
- **Proper logging**: Errors are logged with appropriate severity levels
- **Recovery paths**: Components can recover from recoverable errors

## Thread Safety

PanelKit follows a primarily single-threaded design for simplicity and safety:
- **Main UI thread**: Handles rendering and input processing
- **Thread-safe components**: Core components use Arc/Mutex where needed
- **Event-based communication**: Avoids complex thread synchronization

## Future Extensibility

The architecture is designed to be extended with:
- New page types through the Page trait
- New platform implementations via the PlatformDriver trait
- Advanced input methods (gestures, multi-touch) through the event system
- Additional storage backends through the state manager