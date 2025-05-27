# System Architecture

## Overview

PanelKit follows a modular, layered architecture with clean abstractions between platform-specific code and application logic. The system is designed to run on both development machines (macOS/Linux) and embedded ARM64 targets.

```
┌─────────────────────────────────────────────────────────────────┐
│                     Application Layer (app.c)                   │
│                  Main loop, initialization, lifecycle           │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌─────────────────────────────┴────────────────────────────────────┐
│                         UI Layer                                │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  Widget System  │  │ Widget Manager  │  │ Widget Factory  │ │
│  │  - Base widget  │  │ - Tree mgmt     │  │ - Creation      │ │
│  │  - Rendering    │  │ - Event routing │  │ - Type registry │ │
│  │  - Hit testing  │  │ - Focus mgmt    │  │                 │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                             │
┌─────────────────────────────┴────────────────────────────────────┐
│                     Integration Layer                           │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  Event System   │  │  State Store    │  │ API Manager     │ │
│  │  - Type-safe    │  │ - Key-value     │  │ - HTTP client   │ │
│  │  - Pub/sub      │  │ - Thread-safe   │  │ - JSON parsing  │ │
│  │  - Async        │  │ - TTL support   │  │ - Callbacks     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                             │
┌─────────────────────────────┴────────────────────────────────────┐
│                    Abstraction Layer                            │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │ Display Backend │  │  Input Handler  │  │ Config Manager  │ │
│  │ - SDL standard  │  │ - SDL events    │  │ - YAML parser   │ │
│  │ - SDL+DRM       │  │ - Linux evdev   │  │ - Validation    │ │
│  │ - Auto-detect   │  │ - Mock source   │  │ - Hot reload    │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                             │
┌─────────────────────────────┴────────────────────────────────────┐
│                       Platform Layer                            │
│         SDL2, Linux kernel (DRM/evdev), System libraries        │
└─────────────────────────────────────────────────────────────────┘
```

## Module Responsibilities

### Application Layer (`app.c`)
- SDL initialization and cleanup
- Main event loop
- Component initialization and lifecycle
- Global state coordination (being phased out)

### UI Layer

#### Widget System (`widget.h/c`)
- Base widget structure with common properties
- Rendering pipeline with proper clipping
- Hit testing for touch/mouse events
- Parent-child relationships
- State management (hover, pressed, focused, etc.)

#### Widget Manager (`widget_manager.h/c`)
- Multiple root widget support
- Event routing to appropriate widgets
- Focus management
- Rendering coordination
- Widget lookup by ID

#### Widget Types
- **Container**: Basic container for layout
- **Button**: Interactive button with callbacks
- **Label**: Text display with alignment
- **PageManager**: Multi-page navigation with transitions
- **Time**: Real-time clock display
- **DataDisplay**: Formatted data presentation

### Integration Layer

#### Event System (`event_system.h/c`)
- Thread-safe publish/subscribe mechanism
- Type-safe event wrappers
- Synchronous event delivery
- No event queuing (direct dispatch)

#### State Store (`state_store.h/c`)
- Centralized application state
- Thread-safe key-value storage
- Namespace support (type:id)
- TTL and cache control
- Change notifications

#### API Manager (`api_manager.h/c`)
- High-level API orchestration
- Request lifecycle management
- Callback-based result delivery
- Error handling and retry logic

### Abstraction Layer

#### Display Backend (`display_backend.h/c`)
- Unified display interface
- SDL standard mode (windowed)
- SDL+DRM mode (direct rendering)
- Resolution and capability detection

#### Input Handler (`input_handler.h/c`)
- Strategy pattern for input sources
- SDL native input processing
- Linux evdev for direct touch
- Mock source for testing
- Unified event format

#### Configuration (`config_manager.h/c`)
- YAML configuration loading
- Schema validation
- Default value management
- Runtime override support

## Data Flow

### User Input Flow
1. Hardware → Input source (SDL/evdev)
2. Input handler → Normalized events
3. Widget manager → Event routing
4. Target widget → State update
5. Event system → State change notification
6. Subscribers → UI update

### API Data Flow
1. API manager → HTTP request
2. API client → Network operation
3. JSON parser → Domain objects
4. Callback → State store update
5. Event system → Data change notification
6. Widgets → Display update

### Configuration Flow
1. YAML files → Parser
2. Schema validation → Config structure
3. Config manager → Runtime access
4. Components → Configuration query

## Threading Model

- **Main thread**: UI rendering, event processing
- **API thread**: Network operations (one per request)
- **State store**: Thread-safe with mutex protection
- **Event system**: Thread-safe delivery

## Memory Management

- **Widgets**: Hierarchical ownership (parent owns children)
- **Events**: Copy-on-publish (no reference holding)
- **State store**: Owns all stored data
- **API responses**: Caller owns returned data

## Error Handling

- **PkError**: Unified error code system
- **Context**: Thread-local error context with formatting
- **Propagation**: Return codes bubble up call stack
- **Recovery**: Component-specific strategies
- **Logging**: Automatic error logging integration