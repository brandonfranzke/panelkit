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

### 2. UI Component Design

#### Button Design
- Buttons should be 50% of the window width
- Button height should be 2/3 of the viewport height
- Use center alignment for consistent placement
- Provide clear visual feedback for touches

#### ScrollView Design
- Content should adapt to its contained elements
- Smooth scrolling with appropriate friction
- Visual indicators for scroll position (optional)
- Properly handle event propagation within scrollable areas

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
2. **Volume Mounting**: Share source code and build artifacts with host
3. **Caching**: Optimize build times with Docker volumes for caching
4. **Target-Specific Configurations**: Conditional compilation for platform differences

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
└─────────────────────────────────────────────┘
```

#### Logging Features:

1. **Multiple Log Levels**: ERROR, WARN, INFO, DEBUG, TRACE
2. **Context Enrichment**: Include source context, component, and thread information
3. **Filterable**: Runtime control of log levels by category
4. **Event Logging**: Detailed logging of touch events and UI interactions
5. **Visual Debug Mode**: Optional overlay showing debug information

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
2. **Component Adaptation**: UI components that resize appropriately
3. **Touch Target Sizing**: Ensure minimum touch target sizes regardless of screen resolution

## Deployment Configuration

### Target System Setup

```
┌─────────────────────────────────────────────┐
│             Target Device                   │
│                                             │
│  ┌─────────────────────────────────────────┐│
│  │            Systemd Service              ││
│  │                                         ││
│  │  ┌─────────────┐    ┌──────────────┐   ││
│  │  │ Application │    │Environment   │   ││
│  │  │             │    │Variables     │   ││
│  │  └─────────────┘    └──────────────┘   ││
│  └─────────────────────────────────────────┘│
└─────────────────────────────────────────────┘
```

#### Deployment Features:

1. **Auto-Start**: Application launches on system boot
2. **Environment Configuration**: Proper environment variables for embedded operation
3. **Screen Configuration**: Set dimensions and orientation via command line
4. **Error Recovery**: Automatic restart on crash
5. **Logging**: Capture stdout/stderr to log files

## Implementation Phases

### Phase 1: Project Setup and Core Infrastructure
1. Basic project structure
2. Docker build configuration
3. Logging infrastructure
4. Core platform abstraction

### Phase 2: Component Development
1. Button components
2. Scrollable views
3. Page navigation system
4. Event handling system

### Phase 3: Integration and Refinement
1. Connect all components
2. Optimize performance
3. Testing and debugging
4. Cross-platform validation

## Coding Standards

1. **Clean Code**: Self-documenting code with meaningful names and minimal comments
2. **Error Handling**: Comprehensive error handling
3. **Performance Awareness**: Consider resource constraints on target platform
4. **Simple Makefile**: Maintain a simple build system with clear, focused targets
5. **Relative Sizing**: Use relative measurements (%, fractions) rather than fixed pixel values

## Technical Requirements

- **Rust**: Version 1.70+ recommended
- **Slint**: Version 1.11.0
- **Docker**: For cross-compilation
- **Embedded Target**: Raspberry Pi 4 or similar with touchscreen
- **Screen Resolution**: Default 640x480 in portrait orientation