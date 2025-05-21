# PanelKit Design Documentation

This document consolidates the design principles, architecture, and implementation guidelines for the PanelKit project, now implemented using SDL2.

## Project Objectives and Requirements

PanelKit is designed as a touch-centric UI application targeting both development machines (macOS) and embedded Linux devices (Raspberry Pi). The primary goals are:

1. Create a responsive, touch-first interface for embedded devices
2. Simplify cross-compilation between development and target environments
3. Optimize UI elements for touchscreen interaction
4. Establish a clean, maintainable codebase with minimal dependencies
5. Enable seamless deployment to target hardware

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
- Simple CMake configuration with minimal complexity
- Easily extendable for cross-compilation if needed
- Clear separation between development and deployment commands
- Convenient build script with clean and build targets

### Target Environment Requirements
- LinuxFB backend for direct framebuffer access on embedded targets
- Software renderer for maximum compatibility
- Systemd service for auto-start on boot
- Portrait orientation (640x480) as default
- No external input devices (keyboard/mouse) required on target

## Technical Environment

- **Development**: macOS or Linux
- **Target**: Raspberry Pi or similar with touchscreen
- **Graphics Library**: SDL2 with SDL2_ttf for text rendering
- **Languages**: C
- **Build Tools**: CMake, Make, gcc/clang

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
- Environment variable configuration for different backends
- Command-line arguments for screen dimensions and orientation
- Conditional compilation for platform-specific code

## Deployment Process

1. Build the application for the target platform
2. Deploy binary to the target device
3. Configure systemd service for auto-start
4. Set up appropriate permissions for framebuffer access

## Next Steps and Enhancements

1. Implement cross-compilation support for ARM targets
2. Add configuration file support for customization
3. Enhance API integration with caching and error handling
4. Add support for more complex UI layouts and components
5. Improve internationalization and localization support