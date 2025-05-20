# Panel Kit Application Design Plan

## Project Overview

This document outlines the architecture and design approach for a touch-based UI application targeted primarily for embedded devices (Raspberry Pi CM5) with considerations for development on macOS hosts. The application will serve as an appliance-like interface with direct framebuffer access on the target system.

## Core Requirements

- **Touch-only interface**: No keyboard or mouse input on target device
- **Responsive layout**: Adapt to different screen resolutions at startup (no runtime changes)
- **Multi-page navigation**: Swipe gestures between pages
- **Scrollable content**: Touch and drag to scroll content within pages
- **Basic UI elements**: Buttons, text displays, and simple controls
- **Proper event propagation**: Clear distinction between button presses and scrolling gestures
- **Cross-platform development**: Unified codebase for both host (macOS) and target (Linux on RPi)
- **Containerized build**: Docker-based cross-compilation without host dependencies

## Architecture Overview

The application will be built using a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────┐
│                  Application                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │    Pages    │  │   Widgets   │  │    State    │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
├─────────────────────────────────────────────────────┤
│                UI Framework (Slint)                 │
├─────────────────────────────────────────────────────┤
│           Platform Abstraction Layer                │
├───────────────────┬─────────────┬─────────────────┬─┤
│     MacOS Host    │    Linux    │ Framebuffer     │ │
│  (Slint + winit)  │  (Desktop)  │  (LinuxKMS)     │ │
└───────────────────┴─────────────┴─────────────────┴─┘
```

## Component Design

### 1. Event Handling System

The core challenge identified is properly handling and distinguishing different touch events:

```
┌─────────────────────────────────────────────┐
│               Touch Event                   │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│           Event Classifier                  │
└───────┬─────────────────┬───────────────────┘
        │                 │
        ▼                 ▼
┌───────────────┐ ┌───────────────────────────┐
│  Tap Event    │ │      Gesture Event        │
└───────┬───────┘ └─────────────┬─────────────┘
        │                       │
        ▼                       ▼
┌───────────────┐ ┌─────────────────┬─────────┐
│ Button Action │ │     Swipe       │  Scroll │
└───────────────┘ └─────────────────┴─────────┘
```

#### Key Design Principles for Event Handling:

1. **Early Classification**: Distinguish between taps and gestures early in the event pipeline
2. **Gesture State Machine**: Track touch positions over time to detect swipes vs. scrolls
3. **Event Boundaries**: Define clear boundaries for where events are consumed vs. propagated
4. **Conflict Resolution**: Rules for handling ambiguous events (e.g., touch started on button but turns into drag)
5. **Timeout-Based Disambiguation**: Use time thresholds to distinguish between taps and gesture starts
6. **Velocity Tracking**: Calculate speed and direction of touch movements for inertial scrolling

### 2. Page Navigation System

```
┌─────────────────────────────────────────────┐
│             PageManager                     │
│                                             │
│  ┌─────────────┐  ┌─────────────────────┐   │
│  │ Page Stack  │  │ Transition Manager  │   │
│  └─────────────┘  └─────────────────────┘   │
│                                             │
└─────────────────────────────────────────────┘
            │             │
            ▼             ▼
┌───────────────┐  ┌─────────────────┐
│  Page Content │  │ Swipe Detector  │
└───────────────┘  └─────────────────┘
```

#### Page Navigation Features:

1. **Stack-Based Navigation**: Push/pop pages for hierarchical navigation
2. **Swipe Transitions**: Horizontal swipes to move between pages at the same level
3. **Visual Transitions**: Smooth animations when changing pages
4. **State Preservation**: Maintain page state during navigation
5. **Page Factory**: Create pages on demand based on navigation actions

### 3. Widget System

Reusable, self-contained UI components that handle their own internal state:

```
┌─────────────────────────────────────────────┐
│                 BaseWidget                  │
└─────────────────────────────────────────────┘
         ▲             ▲             ▲
         │             │             │
┌────────┴─────┐ ┌─────┴───────┐ ┌───┴────────┐
│    Button    │ │ TextDisplay │ │ ScrollView │
└──────────────┘ └─────────────┘ └────────────┘
```

#### Widget Principles:

1. **Self-Contained**: Widgets handle their own internal state and appearance
2. **Event Handling**: Clear rules for when events are consumed vs. propagated
3. **Responsive Design**: Adapt to container size and screen resolution
4. **Minimal Dependencies**: Limit dependencies between widgets
5. **Consistent API**: Similar patterns for configuration and callbacks

### 4. Scrolling Implementation

The scrolling view requires special attention:

```
┌─────────────────────────────────────────────┐
│               ScrollView                    │
│                                             │
│  ┌─────────────────────────────────────────┐│
│  │              Viewport                   ││
│  │  ┌─────────────────────────────────────┐││
│  │  │           Content                   │││
│  │  │                                     │││
│  │  └─────────────────────────────────────┘││
│  └─────────────────────────────────────────┘│
└─────────────────────────────────────────────┘
```

#### Scrolling Features:

1. **Touch Drag Detection**: Track touch events to determine scroll intent
2. **Inertial Scrolling**: Continue scrolling with decreasing velocity after touch release
3. **Bounce Effects**: Visual feedback at content boundaries
4. **Event Capture**: Properly handle events within scrollable areas
5. **Variable Sizing**: Adapt to both content size and viewport size
6. **Efficient Rendering**: Only render visible content when possible

## Build System Design

### Docker-Based Cross-Compilation

```
┌─────────────────────────────────────────────┐
│              Host Machine                   │
│                                             │
│  ┌─────────────────────────────────────────┐│
│  │           Docker Container              ││
│  │                                         ││
│  │  ┌─────────────┐    ┌──────────────┐   ││
│  │  │ Rust Toolchain│    │Target Sysroot │   ││
│  │  └─────────────┘    └──────────────┘   ││
│  │           │              │             ││
│  │           ▼              ▼             ││
│  │  ┌─────────────────────────────────────┐││
│  │  │         Cross-Compiled Binary       │││
│  │  └─────────────────────────────────────┘││
│  └─────────────────────────────────────────┘│
└─────────────────────────────────────────────┘
```

#### Build System Features:

1. **Self-Contained Build**: All dependencies included in Docker image
2. **Multi-Stage Builds**: Separate compilation from packaging
3. **Volume Mounting**: Share source code and build artifacts with host
4. **Caching**: Optimize build times with proper caching
5. **Target-Specific Configurations**: Conditional compilation for platform differences
6. **Consistent Versions**: Lock dependencies to specific versions

## Logging and Debugging System

### Comprehensive Logging Infrastructure

```
┌─────────────────────────────────────────────┐
│              Application                    │
│                                             │
│  ┌─────────────┐  ┌───────────┐  ┌────────┐ │
│  │ Error Logs  │  │Info Logs  │  │Debug   │ │
│  └─────────────┘  └───────────┘  │Logs    │ │
│                                  └────────┘ │
│                     │                       │
│                     ▼                       │
│  ┌─────────────────────────────────────────┐│
│  │           Logging System                ││
│  └─────────────────────────────────────────┘│
│                     │                       │
│                     ▼                       │
│  ┌─────────────┐  ┌───────────┐  ┌────────┐ │
│  │ Console     │  │File Output│  │Remote  │ │
│  │ Output      │  │           │  │Logging │ │
│  └─────────────┘  └───────────┘  └────────┘ │
└─────────────────────────────────────────────┘
```

#### Logging Features:

1. **Multiple Log Levels**: ERROR, WARN, INFO, DEBUG, TRACE
2. **Context Enrichment**: Include source context (file/line), component, and thread information
3. **Filterable**: Runtime control of log levels by category
4. **Performance Tracking**: Measure and log performance metrics
5. **Event Logging**: Detailed logging of touch events and UI interactions
6. **Visual Debug Mode**: Overlay showing touch points and event propagation
7. **Conditional Compilation**: More detailed logging in debug builds

## Responsive Design Approach

### Resolution Adaptation Strategy

```
┌─────────────────────────────────────────────┐
│      Resolution Detection at Startup        │
└───────────────────────┬─────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────┐
│             Layout Calculation              │
└───────────────────────┬─────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────┐
│               Component Sizing              │
└─────────────────────────────────────────────┘
```

#### Responsive Design Features:

1. **Layout Units**: Define layouts in relative units rather than absolute pixels
2. **Breakpoints**: Different layouts for significantly different screen sizes
3. **Component Adaptation**: UI components that resize appropriately
4. **Font Scaling**: Adjust text size based on screen resolution
5. **Touch Target Sizing**: Ensure minimum touch target sizes regardless of screen resolution
6. **Aspect Ratio Preservation**: Maintain proper proportions for visual elements

## State Management

For a moderate state management system:

```
┌─────────────────────────────────────────────┐
│                App State                    │
│                                             │
│  ┌─────────────┐  ┌───────────┐  ┌────────┐ │
│  │ UI State    │  │Page State │  │App     │ │
│  │             │  │           │  │Config  │ │
│  └─────────────┘  └───────────┘  └────────┘ │
└─────────────────────────────────────────────┘
              │           │          │
              ▼           ▼          ▼
┌─────────────────────────────────────────────┐
│                  Views                      │
└─────────────────────────────────────────────┘
```

#### State Management Features:

1. **Centralized State**: Single source of truth for application state
2. **Immutable Updates**: State changes through well-defined update functions
3. **Change Notification**: Components receive updates when relevant state changes
4. **State Persistence**: Save and restore state as needed
5. **State Scoping**: Localize state to components when appropriate
6. **Serialization**: Convert state to/from JSON or other formats as needed

## Implementation Phases

### Phase 1: Project Setup and Foundation
1. Basic project structure
2. Docker build configuration
3. Logging infrastructure
4. Core platform abstraction

### Phase 2: Event System Implementation
1. Touch event handling
2. Gesture detection
3. Event propagation rules

### Phase 3: UI Component Framework
1. Base widget implementation
2. Button component
3. Text display component
4. ScrollView implementation

### Phase 4: Page Navigation System
1. Page manager
2. Swipe navigation
3. Page transitions

### Phase 5: Application Integration
1. Sample page layouts
2. State management integration
3. End-to-end testing

## Development Environment

### Development Workflow

```
┌─────────────────────────────────────────────────────────────┐
│                      Development Host (MacOS)                │
│                                                             │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────────┐    │
│  │ Source Code │──▶│ Docker Build│──▶│ Host Binary     │    │
│  └─────────────┘   └─────────────┘   └─────────────────┘    │
│         │                │                    │              │
│         │                │                    ▼              │
│         │                │           ┌─────────────────┐    │
│         │                │           │ Local Testing   │    │
│         │                │           └─────────────────┘    │
│         │                │                                  │
│         │                ▼                                  │
│         │       ┌─────────────────┐                        │
│         │       │ Target Binary   │                        │
│         │       └─────────────────┘                        │
│         │                │                                  │
│         ▼                ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                Version Control                       │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
            │                                    │
            │                                    │
            ▼                                    ▼
┌─────────────────────────┐        ┌───────────────────────────┐
│ Target Device           │        │ Continuous Integration    │
│ (Raspberry Pi CM5)      │        │                           │
└─────────────────────────┘        └───────────────────────────┘
```

### Testing Approach

1. **Manual Testing**: Initial testing of UI interactions
2. **Integration Testing**: Test components working together
3. **Visual Testing**: Compare UI rendering across platforms
4. **Performance Testing**: Measure response times and resource usage

## Technical Specifications

### Slint UI Framework Integration

1. **Component Definition**: Define UI components using Slint's declarative language
2. **Event Binding**: Connect Slint events to Rust handlers
3. **Property Binding**: Two-way binding between UI and application state
4. **Custom Rendering**: Platform-specific rendering optimizations when needed
5. **Backend Selection**: Proper backend selection based on target platform

### Cross-Platform Considerations

1. **Conditional Compilation**: Use feature flags for platform-specific code
2. **Framebuffer Access**: Direct framebuffer access on target via LinuxKMS
3. **Input Sources**: Unified input handling for touch events from different sources
4. **Performance Profiling**: Platform-specific performance measurements
5. **Resource Management**: Appropriate resource usage for platform capabilities

## Coding Standards

1. **Modular Design**: Clear separation of concerns with well-defined interfaces
2. **Clean Code**: Self-documenting code with meaningful names
3. **Error Handling**: Comprehensive error handling without silent failures
4. **Documentation**: Thorough documentation, including architecture decisions
5. **Consistent Style**: Follow Rust idioms and code style guidelines
6. **Performance Awareness**: Consider performance implications of design choices

## Future Considerations

1. **Theming Support**: Framework for visual theming and customization
2. **Plugin System**: Extension points for additional functionality
3. **Automated Testing**: UI automation testing infrastructure
4. **State Persistence**: Saving application state between sessions
5. **Remote Debugging**: Tools for debugging on target device
6. **Performance Optimization**: Platform-specific optimizations

## Conclusion

This design plan outlines a robust architecture for a touch-based UI application that prioritizes proper event handling, scalable component design, and cross-platform compatibility. By focusing on software engineering principles and a clean architecture, the application will provide a solid foundation for future enhancements while addressing the core requirements of touch interaction and multi-page navigation.