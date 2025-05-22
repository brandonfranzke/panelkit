# PanelKit Design Documentation

This document consolidates the design principles, architecture, and implementation guidelines for the PanelKit project, implemented using SDL2 for embedded Linux devices.

## Project Objectives and Requirements

PanelKit is designed as a touch-centric UI application targeting both development machines (macOS/Linux) and embedded Linux devices (Raspberry Pi CM5). The primary goals are:

1. Create a responsive, touch-first interface for embedded devices
2. Simplify cross-compilation between development and target environments  
3. Optimize UI elements for touchscreen interaction
4. Establish a clean, maintainable codebase with minimal dependencies
5. Enable seamless deployment to target hardware
6. Provide reliable logging and debugging capabilities

## Architecture Overview

The application uses a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────┐
│                  Application                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │    Pages    │  │   Widgets   │  │    State    │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
├─────────────────────────────────────────────────────┤
│               Graphics System (SDL2)                │
├─────────────────────────────────────────────────────┤
│           Platform Abstraction Layer                │
├───────────────────┬─────────────┬─────────────────┬─┤
│     MacOS Host    │    Linux    │ Framebuffer     │ │
│      (SDL2)       │  (Desktop)  │  (LinuxFB)      │ │
└───────────────────┴─────────────┴─────────────────┴─┘
```

## UI Design and Requirements

### Button Design Requirements
- Buttons should be 50% of the viewport width
- Button height should be 2/3 of the viewport height
- Buttons should be center-aligned in the layout
- Each button needs clear visual feedback for touches (normal, hover, pressed, held states)

### Layout Considerations
- The application should support portrait orientation (640x480)
- UI should adapt to different screen dimensions without fixed pixel values
- Scrollable areas must handle touch events properly
- Background color should be customizable with no visual artifacts

### UI Interaction Design
- Multi-page navigation with swipe gestures
- Scrollable content within pages
- Touch and drag to scroll content
- Tap to activate buttons
- Clear visual indicators for page navigation (page indicators)

## Event Handling System

The core of the application is a robust event handling system that can properly distinguish between different types of touch interactions:

### Event Classification

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

### Key Design Principles for Event Handling:

1. **State Machine**: Use a state machine approach to track gesture states (NONE, POTENTIAL, CLICK, DRAG_VERT, DRAG_HORZ, HOLD)
2. **Early Classification**: Distinguish between taps and gestures early in the event pipeline
3. **Timeout-Based Disambiguation**: Use time thresholds to distinguish between taps and holds
4. **Distance Thresholds**: Use pixel distance thresholds to distinguish between taps and drags
5. **Directional Detection**: Determine whether a drag is horizontal (page swipe) or vertical (content scroll)
6. **Event Boundaries**: Define clear boundaries for where events are consumed vs. propagated

## Page System

The application uses a multi-page system with smooth transitions:

1. **Page Structure**: Each page has its own content, scroll position, and UI elements
2. **Transition Animations**: Smooth sliding animations between pages
3. **Page Indicators**: Visual indicators showing current page and total pages
4. **Content Scrolling**: Each page handles its own vertical scrolling

## API Integration

The application demonstrates integration with external services:

1. **HTTP Requests**: Using libcurl for API communication
2. **Threading**: Background processing of API requests
3. **Data Display**: Rendering API data in the UI
4. **Refresh Mechanisms**: Both automatic and manual refresh options

## Build System Approach

The build system is designed with these principles:

### Development and Cross-Compilation
- **Host builds**: Fast iteration for development using `build/host/` directory
- **Target builds**: ARM64 cross-compilation using Docker in `build/target/` directory
- **Separate CMake caches**: Prevents conflicts when switching between host and target builds
- **Font embedding**: Integrated as build prerequisite with configurable font selection

### Deployment Automation
- **Long option arguments**: Clear command-line interface using `--host`, `--user`, `--target-dir`
- **SSH configuration**: Support for SSH config files and optional user specification
- **File-based deployment**: Copy files to target without automatic installation
- **Target-side Makefile**: Separate Makefile on target for setup, installation, and service management

### Configuration Management
- **Variable-driven Makefile**: Centralized configuration with reasonable defaults
- **Font selection**: Easy switching between embedded fonts
- **Target customization**: Override deployment settings per environment

## Target Environment Requirements

### System Configuration
- **Direct framebuffer**: LinuxFB backend for direct framebuffer access on embedded targets
- **Software renderer**: Maximum compatibility without GPU dependencies
- **Systemd service**: Auto-start on boot with proper restart policies
- **File-based logging**: Logs to `/var/log/panelkit/panelkit.log` for remote debugging

### Runtime Environment
- **Portrait orientation**: Default 640x480 resolution
- **No external input**: Keyboard/mouse not required on target
- **Minimal dependencies**: Statically linked where possible
- **Permission setup**: Automated configuration for framebuffer and video group access

## Technical Environment

- **Development**: macOS or Linux with Docker support
- **Target**: ARM64 Linux (Raspberry Pi CM5)
- **Graphics Library**: SDL2 with SDL2_ttf for text rendering
- **Languages**: C with minimal external dependencies
- **Build Tools**: CMake, Make, Docker, gcc/clang

## Code Style Direction

1. Prefer self-documenting code with minimal comments
2. Use relative sizing (%, fractions) over fixed pixel values
3. Keep the build system simple and easy to understand
4. Avoid unnecessary abstractions and dependencies
5. Use clear, descriptive variable and function names

## Implementation Components

### Core Components
- **Gesture Detection**: State machine for classifying touch inputs
- **Page System**: Management of multiple pages with transitions
- **ScrollView Implementation**: Vertical scrolling within pages
- **Button System**: Interactive buttons with visual feedback
- **API Integration**: Background API requests with threading

### Platform Adaptation
- **Environment variable configuration**: Different backends for different platforms
- **Command-line arguments**: Screen dimensions and orientation configuration
- **Conditional compilation**: Platform-specific code where necessary

## Deployment Process

### Build Phase
1. **Font embedding**: Generate embedded font header
2. **Cross-compilation**: Build ARM64 binary using Docker
3. **Validation**: Verify binary dependencies and architecture

### Deployment Phase
1. **File transfer**: Copy binary and deployment files to target
2. **Permission setup**: Configure system permissions and groups
3. **Service installation**: Install and enable systemd service
4. **Service management**: Start, stop, monitor application

### Debugging and Monitoring
1. **File-based logging**: Easy access to logs for troubleshooting
2. **Service status**: Standard systemd commands for monitoring
3. **Remote debugging**: Log files can be easily transferred for analysis

## Next Steps and Enhancements

1. **Enhanced API integration**: Add caching and error handling
2. **Configuration file support**: Runtime customization without rebuilds
3. **Additional UI components**: More complex layouts and interactions
4. **Performance optimizations**: Ensure smooth operation on constrained hardware
5. **Internationalization**: Improved Unicode and localization support