# PanelKit SDL

A touch-optimized UI application built with SDL2 for embedded Linux devices. Features a multi-page interface with unified touch/mouse input handling, comprehensive input abstraction system, and **SDL+DRM rendering** for minimal dependencies on Raspberry Pi and similar embedded targets.

## Features

- **Unified Input System**: Touch and mouse events use the same gesture recognition code for consistent behavior between development and production
- **Multi-page Interface**: Smooth swipe transitions with iPhone-style page indicators  
- **Gesture Recognition**: Tap, drag, swipe, and scroll detection with configurable thresholds
- **Input Abstraction**: Pluggable input sources (SDL native, Linux evdev, mock) with automatic device detection
- **SDL+DRM Backend**: Direct framebuffer rendering with minimal dependencies (no X11/Wayland required)
- **Cross-platform Build**: Development on macOS/Linux, deployment to ARM64 embedded targets
- **Comprehensive Logging**: Structured logging with build info and system introspection
- **Systemd Integration**: Service-based deployment with automatic restart and log management
- **API Integration**: Background data fetching with threading and JSON parsing

## Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Application    │    │ Display Backend │    │ Input Handler   │
│  - Event Loop   │◄──►│ - SDL+DRM       │◄──►│ - Evdev Source  │
│  - UI Rendering │    │ - Standard SDL  │    │ - SDL Source    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
        │                       │                       │
        ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Gesture System  │    │   Core Utils    │    │  API Client     │
│ - Touch/Mouse   │    │ - Logging       │    │ - HTTP Client   │
│ - State Machine │    │ - Build Info    │    │ - Threading     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Project Structure

```
panelkit_sdl/
├── src/                    # Source code
│   ├── app.c              # Main application with unified input handling
│   ├── api_functions.c    # API integration and data fetching
│   ├── core/              # Core utilities
│   │   ├── logger.c       # Structured logging system
│   │   ├── build_info.c   # Build information and introspection
│   │   └── sdl_includes.h # Centralized SDL header management
│   ├── display/           # Display backend abstraction
│   │   ├── display_backend.h    # Backend interface
│   │   ├── backend_sdl.c        # Standard SDL backend
│   │   └── backend_sdl_drm.c    # SDL+DRM backend
│   └── input/             # Input handling abstraction
│       ├── input_handler.h      # Input handler interface
│       ├── input_source_evdev.c # Linux evdev input source
│       ├── input_source_sdl.c   # SDL native input source
│       ├── input_source_mock.c  # Mock input for testing
│       └── input_debug.c        # Input system debugging
├── test/                  # Comprehensive test suite
│   ├── input/             # Input system tests
│   │   ├── test_touch_raw.c     # Raw Linux touch testing
│   │   ├── test_sdl_drm_touch.c # Complete SDL+DRM+Touch solution
│   │   └── TESTING_PLAN.md      # Systematic testing methodology
│   └── core/              # Core system tests
├── scripts/               # Build and deployment automation
│   ├── build_host.sh      # Host development build
│   ├── build_target.sh    # ARM64 cross-compilation with Docker
│   ├── deploy.sh          # Target deployment automation
│   └── debug_touch.sh     # Comprehensive touch debugging utility
├── deploy/                # Target device configuration
│   ├── Makefile          # Target setup and service management
│   ├── panelkit.service  # Systemd service definition
│   └── README.md         # Deployment instructions
├── fonts/                 # Font management and embedding
├── docs/                  # Technical documentation
└── CMakeLists.txt        # Cross-platform build configuration
```

## Building

### Development (Host)
```bash
make host           # Build for development/testing (macOS/Linux)
```

### Target (ARM64 Cross-compilation)
```bash
make target         # Cross-compile for ARM64 Linux using Docker
```

### Font Management
```bash
make font           # Embed default font
make font DEFAULT_FONT=font-sans-dejavu.ttf  # Use specific font
```

### Clean Build
```bash
make clean          # Remove all build artifacts
```

## Input System

The unified input handling system ensures consistent behavior across platforms:

- **Development**: Mouse events translated to touch events
- **Production**: Direct touch event handling via evdev or SDL
- **Consistent Gestures**: Same code path for all input types
- **Automatic Detection**: Input source selected based on display backend

### Touch Event Flow
```
Hardware Touch → Linux evdev → Input Handler → Unified Touch Events → Gesture Recognition → UI Actions
Mouse Input    → SDL Events  → Input Handler → Unified Touch Events → Gesture Recognition → UI Actions
```

## Display Backends

### SDL+DRM Backend (Recommended for Embedded)
- **Dependencies**: libdrm only (~200KB)
- **Benefits**: Minimal dependencies, hardware-synchronized, no tearing
- **Use Case**: Embedded devices without X11/Wayland

### Standard SDL Backend (Development)
- **Dependencies**: Full SDL2 stack
- **Benefits**: Cross-platform, easy debugging
- **Use Case**: Development and testing

## Deployment

Deploy to target device:

```bash
# Basic deployment
make deploy TARGET_HOST=panelkit

# With custom configuration
make deploy TARGET_HOST=192.168.1.100 TARGET_USER=pi TARGET_PATH=/opt/panelkit

# Direct script usage with long options
./scripts/deploy.sh --host panelkit --user pi --target-dir /tmp/panelkit
```

### Target Device Setup

After deployment, on the target device:

```bash
cd /tmp/panelkit
make setup          # Setup permissions and directories
make install        # Install binary and systemd service
make start          # Start the service
make logs           # View logs in real-time
```

## Touch Input Debugging

Use the comprehensive debug script:

```bash
# On target device
./scripts/debug_touch.sh
```

Features:
- Permission checking (input/video groups)
- Touch device auto-detection
- Raw input event testing
- PanelKit integration testing
- Detailed troubleshooting guidance

## Testing

The project includes a comprehensive test suite:

```bash
# Deploy and build input tests on target
cd test
make deploy-input

# Run specific tests on target
ssh user@target 'cd /tmp/panelkit-tests && sudo ./test_touch_raw'
ssh user@target 'cd /tmp/panelkit-tests && ./test_sdl_drm_touch'
```

## Dependencies

### Build Dependencies
- **Host**: CMake, GCC/Clang, pkg-config
- **Cross-compilation**: Docker
- **Development**: SDL2, SDL2_ttf, libcurl development packages

### Runtime Dependencies
- **SDL+DRM Mode**: libdrm (~200KB), minimal system libraries
- **Standard Mode**: SDL2, SDL2_ttf, libcurl, system graphics stack
- **Target System**: ARM64 Linux with systemd

## Controls

- **Touch/Mouse**: Unified input handling
- **Horizontal Swipe**: Navigate between pages
- **Vertical Swipe**: Scroll content within pages
- **Tap**: Activate buttons and UI elements
- **Keyboard** (development):
  - **D**: Toggle debug overlay
  - **Arrow Keys**: Page navigation and scrolling
  - **ESC**: Exit application

## Configuration

### Build Configuration
Key variables in `Makefile`:
- `TARGET_HOST`: SSH hostname or IP address
- `TARGET_USER`: SSH user (uses SSH config if empty)
- `TARGET_PATH`: Deployment directory on target
- `DEFAULT_FONT`: Font file to embed

### Runtime Configuration
The application auto-detects the environment and selects appropriate backends:
- **Embedded Environment**: Automatically uses SDL+DRM backend with evdev input
- **Development Environment**: Uses standard SDL backend with mouse/keyboard input

## Logging

Structured logging system with multiple levels:
- **Location**: `/var/log/panelkit/panelkit.log` on target
- **Features**: Build info, system introspection, input event tracing
- **Remote Access**: Easy to transfer and analyze logs

## Pages

- **Page 1**: Text display with customizable colors and API data
- **Page 2**: Interactive buttons demonstrating gesture recognition

## Technical Highlights

1. **Zero-Copy Touch Input**: Direct evdev to SDL event injection
2. **Static Linking**: Minimal runtime dependencies for embedded deployment
3. **Cross-Platform SDL Headers**: Centralized include management for different platforms
4. **Comprehensive Testing**: Systematic validation from hardware drivers to application UI
5. **Production-Ready**: Systemd service with automatic restart and logging

## Troubleshooting

Common issues and solutions:

### Touch Not Working
1. Run `./scripts/debug_touch.sh` on target device
2. Check user permissions (input group membership)
3. Verify touch device detection in logs
4. Test raw input with `test_touch_raw`

### Build Issues
1. Ensure all development packages are installed
2. Check CMake configuration output
3. For cross-compilation, verify Docker is running
4. Check include paths for SDL headers

### Deployment Issues
1. Verify SSH key authentication
2. Check target device permissions
3. Ensure systemd is available on target
4. Review deployment logs for specific errors

For detailed technical information, see `ARCHITECTURE.md` and the `docs/` directory.