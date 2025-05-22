# PanelKit SDL

A touch-optimized UI application built with SDL2 for embedded Linux devices. Features a multi-page interface with gesture recognition, API integration, and **SDL+DRM rendering** for minimal dependencies on Raspberry Pi CM5.

## Features

- Multi-page interface with smooth swipe transitions
- iPhone-style page indicators  
- Vertical scrolling for content
- API integration with randomuser.me
- Gesture detection (tap, drag, swipe)
- Fixed content areas with scrolling buttons
- Responsive UI elements
- Cross-compilation support for ARM64 targets
- Systemd service integration
- File-based logging for debugging

## Project Structure

```
panelkit_sdl/
├── src/                    # Source files
│   ├── app.c              # Main application
│   ├── api_functions.c    # API integration
│   └── embedded_font.h    # Embedded font data
├── scripts/               # Build and deployment scripts
│   ├── build_host.sh      # Host development build
│   ├── build_target.sh    # ARM64 cross-compilation
│   └── deploy.sh          # Deployment automation
├── fonts/                 # Font files and embedding
├── deploy/                # Target device files
│   ├── Makefile          # Target setup commands
│   ├── panelkit.service  # Systemd service
│   └── README.md         # Deployment guide
├── test_drm/             # SDL+DRM integration (PRODUCTION READY)
│   ├── sdl_drm_renderer.h/c  # Core SDL+DRM library
│   ├── example_usage.c   # Clean integration example
│   └── README.md         # Full technical documentation
├── Makefile              # Main build system
└── CMakeLists.txt        # CMake configuration
```

## Building

### Development (Host)
```bash
make host           # Build for development/testing
```

### Target (ARM64 Cross-compilation)
```bash
make target         # Cross-compile for ARM64 Linux
```

### Font Management
```bash
make font           # Embed default font
make font DEFAULT_FONT=font-sans-dejavu.ttf  # Use different font
```

### Clean Build
```bash
make clean          # Remove all build artifacts
```

## Deployment

Deploy to target device using long options for clarity:

```bash
# Basic deployment
make deploy TARGET_HOST=panelkit

# With custom user and directory
make deploy TARGET_HOST=192.168.1.100 TARGET_USER=pi TARGET_PATH=/opt/panelkit

# Direct script usage
./scripts/deploy.sh --host panelkit --user pi --target-dir /tmp/panelkit
```

### Target Device Setup

After deployment, on the target device:

```bash
cd /tmp/panelkit
make setup          # Setup permissions and directories
make install        # Install binary and systemd service
make start          # Start the service
make logs           # View logs
```

## Graphics Solutions

### SDL+DRM Integration (Recommended)
**PRODUCTION READY** - Minimal dependencies with proper timing
- **Dependencies**: libdrm only (~200KB)
- **Benefits**: Hardware-synchronized, no tearing, static linking
- **Trade-offs**: Software rendering, Linux DRM only
- **See**: `test_drm/README.md` for full technical details

### Standard SDL2+KMSDRM (Alternative)
- **Dependencies**: Mesa+GBM stack (~169MB)
- **Benefits**: Hardware acceleration, cross-platform
- **Trade-offs**: Large dependency chain

## Dependencies

- **Build**: CMake, Docker (for cross-compilation)  
- **Runtime (SDL+DRM)**: libdrm only (~200KB)
- **Runtime (Standard)**: SDL2, SDL2_ttf, libcurl, Mesa+GBM
- **Target**: ARM64 Linux, systemd

## Controls

- **Swipe horizontally**: Navigate between pages
- **Swipe vertically**: Scroll content  
- **Tap**: Activate buttons
- **D key**: Toggle debug overlay
- **ESC key**: Exit application

## Logging

Logs are written to `/var/log/panelkit/panelkit.log` for easy remote debugging and analysis.

## Configuration

Key variables in `Makefile`:
- `TARGET_HOST`: SSH hostname or IP
- `TARGET_USER`: SSH user (optional, uses SSH config if empty)
- `TARGET_PATH`: Deployment directory
- `DEFAULT_FONT`: Font to embed

## Pages

- **Page 1**: Text display with customizable color
- **Page 2**: Interactive buttons and API data display