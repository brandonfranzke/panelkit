# PanelKit Architecture

This document describes the core architecture of the PanelKit SDL implementation, focusing on the current organized structure and deployment workflow.

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

## Project Structure

The project follows standard C conventions with organized directories:

```
panelkit_sdl/
├── src/                    # Source code
│   ├── app.c              # Main application logic
│   ├── api_functions.c    # API integration and threading
│   └── embedded_font.h    # Generated font data
├── scripts/               # Build and deployment automation
│   ├── build_host.sh      # Development builds (build/host/)
│   ├── build_target.sh    # Cross-compilation (build/target/)
│   └── deploy.sh          # Deployment with long options
├── fonts/                 # Font management
│   ├── embed_font.sh      # Font embedding script
│   └── *.ttf             # Font files
├── deploy/                # Target device files
│   ├── Makefile          # Target setup and service management
│   ├── panelkit.service  # Systemd service definition
│   └── README.md         # Deployment instructions
├── build/                 # Build outputs (gitignored)
│   ├── host/             # Development builds
│   └── target/           # Cross-compiled ARM64 binaries
├── Makefile              # Main build system
└── CMakeLists.txt        # CMake configuration
```

## Core Components

### 1. Application Core (src/app.c)

The central component that orchestrates the entire system:
- Initializes SDL2 and other subsystems
- Manages the application lifecycle
- Runs the main event loop
- Handles input events and state tracking

### 2. Page System (src/app.c)

Manages the user interface pages:
- **Page structure**: Contains data for different types of pages
- **Transition system**: Handles smooth animations between pages
- **Page indicators**: Visual feedback showing current page
- **Content management**: Each page maintains its own content and state

### 3. Event Handling System (src/app.c)

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

### 4. UI Components (src/app.c)

Reusable UI elements:
- **Buttons**: Interactive elements with visual feedback
- **ScrollView**: Content that can be scrolled vertically
- **Text rendering**: Display of various text elements using embedded fonts
- **Page indicators**: Visual indicators showing navigation status

### 5. API Integration (src/api_functions.c)

Manages external data:
- **HTTP client**: Using libcurl for API requests
- **Threading**: Background processing to keep UI responsive
- **JSON parsing**: Simple parsing of API responses
- **Data display**: Rendering of API data in the UI

## Build System Architecture

### Development vs. Production Builds

The build system uses separate directories to avoid CMake cache conflicts:

- **Host builds** (`build/host/`): Fast development iteration on macOS/Linux
- **Target builds** (`build/target/`): ARM64 cross-compilation using Docker

### Font Embedding System

Fonts are embedded at build time:
1. `fonts/embed_font.sh` converts TTF to C header
2. Build system automatically embeds default font as prerequisite
3. Configurable font selection via `DEFAULT_FONT` variable

### Cross-Compilation

ARM64 builds use Docker for consistent environment:
- `Dockerfile.target` defines build environment
- Static linking for minimal target dependencies
- Automated binary validation and dependency checking

## Deployment Architecture

### File-Based Deployment

The deployment system copies files without automatic installation:

```bash
./scripts/deploy.sh --host panelkit --user pi --target-dir /tmp/panelkit
```

### Target-Side Management

Once deployed, the target device has its own Makefile for:
- **Setup**: System permissions and directory creation
- **Installation**: Moving files to final locations and configuring systemd
- **Service Management**: Start, stop, restart, status checking
- **Logging**: File-based logs for remote debugging

### Service Architecture

The systemd service is configured for:
- **Auto-restart**: Automatic restart on failures
- **File logging**: Logs to `/var/log/panelkit/panelkit.log`
- **Environment setup**: SDL2 framebuffer configuration
- **Permission management**: Runs as root for hardware access

## Cross-Platform Strategy

PanelKit achieves cross-platform compatibility through SDL2:

1. **SDL2 abstraction**: Handles platform differences in rendering and input
2. **Conditional compilation**: Platform-specific code can be selectively included
3. **Framebuffer support**: Uses SDL2's framebuffer backend on embedded targets
4. **Input abstraction**: Maps different input devices to a common event model

## Error Handling and Logging

### File-Based Logging
- **Target logs**: `/var/log/panelkit/panelkit.log`
- **Remote debugging**: Easy to transfer log files for analysis
- **Standard tools**: Compatible with `tail`, `grep`, `less`

### Error Handling Strategy
- **Graceful degradation**: Main loop continues despite non-critical errors
- **Visual feedback**: User is informed of relevant errors
- **Service restart**: Systemd automatically restarts on failures

## Key Design Patterns

1. **State machine**: Event handling uses state transitions to track gestures
2. **Separation of concerns**: Each component handles a specific responsibility
3. **Event propagation**: Clear rules for when events are consumed vs. propagated
4. **Component-based UI**: Reusable UI elements with consistent behavior
5. **Threading model**: Background processing for non-UI tasks
6. **Configuration-driven**: Variable-driven build system with reasonable defaults

## Deployment Workflow

### Development Cycle
1. **Development**: Build and test with `make host`
2. **Cross-compilation**: Build target with `make target`
3. **Deployment**: Deploy with `make deploy` or script directly
4. **Target setup**: Run setup commands on target device
5. **Service management**: Install and start systemd service

### Long Option Interface

All deployment uses clear long options:
- `--host`: Target hostname or IP
- `--user`: SSH user (optional)
- `--target-dir`: Deployment directory

### Service Management

On target device:
```bash
cd /tmp/panelkit
make setup      # Configure permissions
make install    # Install service
make start      # Start application
make logs       # View logs
```

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
2. **Performance optimization**: Ensure smooth operation on Raspberry Pi CM5
3. **Enhanced logging**: Add structured logging with different verbosity levels
4. **Configuration files**: Runtime customization without rebuilds
5. **Additional UI components**: Implement more interactive elements