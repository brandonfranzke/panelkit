# PanelKit Architecture

This document describes the core architecture of the PanelKit SDL implementation, based on the design principles and goals from the original project.

## System Overview

PanelKit is a lightweight UI application designed for embedded Linux devices with touchscreens. It operates directly on the framebuffer through SDL2, without requiring a window manager or desktop environment.

The architecture follows a modular design with clean separation of concerns:

```
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│  Application  │     │  Page System  │     │  UI Components│
│  - Core loop  │◄───►│  - Navigation │◄───►│  - Buttons    │
│  - Lifecycle  │     │  - Layout     │     │  - Scrolling  │
└───────┬───────┘     └───────┬───────┘     └───────────────┘
        │                     │
        ▼                     ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│ Event System  │     │ SDL2 Platform │     │ API System    │
│ - Gestures    │◄───►│ - Display     │◄───►│ - Data fetch  │
│ - Input       │     │ - Input       │     │ - Threading   │
└───────────────┘     └───────────────┘     └───────────────┘
```

## Core Components

### 1. Application Core (app.c)

The central component that orchestrates the entire system:
- Initializes SDL2 and other subsystems
- Manages the application lifecycle
- Runs the main event loop
- Handles input events and state tracking

### 2. Page System (app.c)

Manages the user interface pages:
- **Page structure**: Contains data for different types of pages
- **Transition system**: Handles smooth animations between pages
- **Page indicators**: Visual feedback showing current page
- **Content management**: Each page maintains its own content and state

### 3. Event Handling System (app.c)

Implements a state machine for gesture detection:
- **Gesture detection**: Distinguishes between taps, drags, and holds
- **Event classification**: Routes input to appropriate handlers
- **Timeout-based disambiguation**: Uses time thresholds to differentiate gestures
- **Distance thresholds**: Uses pixel distance to classify movement types
- **Direction detection**: Determines horizontal (page swipe) vs vertical (content scroll) movements

#### Gesture State Machine

The event system uses a state machine to track and classify input events:

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

States in the gesture state machine:
- **GESTURE_NONE**: No gesture in progress
- **GESTURE_POTENTIAL**: Touch press detected, waiting to classify
- **GESTURE_CLICK**: Quick press and release (under thresholds)
- **GESTURE_DRAG_VERT**: Vertical dragging (for content scrolling)
- **GESTURE_DRAG_HORZ**: Horizontal dragging (for page swiping)
- **GESTURE_HOLD**: Press and hold (over time threshold)

#### Event Propagation Principles

The event system follows these key principles:
1. **Early Classification**: Distinguish between taps and gestures early in the event pipeline
2. **Context-Aware Routing**: Route events based on their location and the UI elements at that position
3. **Conflict Resolution**: Handle ambiguous events (e.g., touch started on button but turns into drag)
4. **Consumption Rules**: Events are consumed when they trigger a specific action, preventing further propagation
5. **Delegation**: UI components can delegate event handling to parent components when appropriate

The propagation cycle typically follows:
1. **Capture Phase**: Events travel from the application to target component
2. **Target Phase**: Event is processed by the target component (e.g., button)
3. **Bubble Phase**: If not consumed, event bubbles back to parent components

### 4. UI Components (app.c)

Reusable UI elements:
- **Buttons**: Interactive elements with visual feedback
- **ScrollView**: Content that can be scrolled vertically
- **Text rendering**: Display of various text elements
- **Page indicators**: Visual indicators showing navigation status

### 5. API Integration (api_functions.c)

Manages external data:
- **HTTP client**: Using libcurl for API requests
- **Threading**: Background processing to keep UI responsive
- **JSON parsing**: Simple parsing of API responses
- **Data display**: Rendering of API data in the UI

## Key Design Patterns

1. **State machine**: Event handling uses state transitions to track gestures
2. **Separation of concerns**: Each component handles a specific responsibility
3. **Event propagation**: Clear rules for when events are consumed vs. propagated
4. **Component-based UI**: Reusable UI elements with consistent behavior
5. **Threading model**: Background processing for non-UI tasks

## Cross-Platform Strategy

PanelKit achieves cross-platform compatibility through SDL2:

1. **SDL2 abstraction**: Handles platform differences in rendering and input
2. **Conditional compilation**: Platform-specific code can be selectively included
3. **Framebuffer support**: Uses SDL2's framebuffer backend on embedded targets
4. **Input abstraction**: Maps different input devices to a common event model

## Error Handling

The application uses a comprehensive error handling strategy:
- **Logging**: Detailed logging at different verbosity levels
- **Graceful degradation**: Main loop continues despite non-critical errors
- **Visual feedback**: User is informed of relevant errors

## Build System

The build system provides flexibility for different environments:

1. **CMake-based build**: Standard build system for C/C++ projects
2. **Docker cross-compilation**: Self-contained environment for building ARM targets
3. **Deployment automation**: Scripts for deploying to embedded targets
4. **Development workflow**: Fast iteration during development

## Future Extensibility

The architecture is designed to be extended with:

1. **New API integrations**: Additional data sources and services
2. **Enhanced UI components**: More sophisticated interactive elements
3. **Advanced gestures**: Multi-touch and complex gesture recognition
4. **Configuration system**: User-customizable settings
5. **Persistent storage**: Saving application state between sessions

## Next Steps

Immediate development priorities:

1. **Cross-compilation testing**: Verify deployment on actual embedded targets
2. **Unicode support**: Improve text rendering for international characters
3. **API caching**: Add local storage for API responses
4. **New UI components**: Implement additional interactive elements
5. **Performance optimizations**: Ensure smooth operation on constrained hardware