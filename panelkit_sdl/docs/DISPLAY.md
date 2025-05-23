# PanelKit Display System

## Overview

PanelKit uses a flexible display backend system that allows the same application code to run on different display technologies without modification. This abstraction enables:

- Development on desktop systems with standard SDL
- Deployment on embedded systems using SDL+DRM
- Future extensibility for new display technologies

## Architecture

```
Application Code
      ↓
Display Backend API (display_backend.h)
      ↓
Backend Implementation
   ├── Standard SDL (desktop/development)
   └── SDL+DRM (embedded/production)
```

## Display Backends

### Standard SDL Backend
- **Use Case**: Development and testing on desktop systems
- **Dependencies**: Standard SDL2 libraries
- **Features**: Window management, hardware acceleration, vsync
- **Platform**: macOS, Linux with X11/Wayland, Windows

### SDL+DRM Backend
- **Use Case**: Production deployment on Raspberry Pi and embedded Linux
- **Dependencies**: libdrm only (~200KB)
- **Features**: Direct hardware access, no window manager needed
- **Platform**: Linux with DRM/KMS support

## Usage

### Command Line Selection

```bash
# Use standard SDL backend (default on desktop)
./panelkit --display-backend sdl

# Use SDL+DRM backend (default on embedded)
./panelkit --display-backend sdl_drm

# Auto-detect best backend
./panelkit  # or --display-backend auto
```

### Environment Variable

```bash
# Force a specific backend
export PANELKIT_DISPLAY_BACKEND=sdl_drm
./panelkit
```

### Auto-Detection Logic

The system automatically selects the best backend:
1. Checks `PANELKIT_DISPLAY_BACKEND` environment variable
2. On Linux without DISPLAY/WAYLAND_DISPLAY → SDL+DRM
3. Otherwise → Standard SDL

## Implementation Details

### SDL+DRM Backend

The SDL+DRM backend provides display output with minimal dependencies:

1. **SDL Rendering**: Uses SDL's offscreen driver for software rendering
2. **DRM Output**: Copies rendered frames to DRM dumb buffers
3. **Direct Display**: Hardware presents buffers without compositor

**Benefits**:
- No Mesa/GBM dependencies (saves ~169MB)
- Hardware-synchronized timing (no tearing)
- Suitable for embedded touchscreen applications

**Limitations**:
- Software rendering only (no GPU acceleration)
- Linux-specific (requires DRM/KMS)
- Fixed to display's native resolution

### Display Adaptation

The display backend reports actual display dimensions, which may differ from requested:

```c
if (display_backend->actual_width != SCREEN_WIDTH) {
    // Application should adapt to actual display size
}
```

## Building

### Development (Host)
```bash
make host  # Standard SDL backend for development
```

### Production (Target)
```bash
make target  # Includes SDL+DRM support, statically linked
```

### Dependencies

**Development**:
- SDL2 (dynamic)
- SDL2_ttf (dynamic)
- libcurl (dynamic)
- zlog (dynamic)

**Production**:
- libdrm (dynamic, ~200KB)
- All others statically linked

## Troubleshooting

### SDL+DRM Issues

**No display output**:
1. Check DRM device permissions: `ls -la /dev/dri/`
2. Verify KMS is enabled: `dmesg | grep drm`
3. Try different device: `ls /dev/dri/card*`

**Permission denied**:
```bash
# Add user to video group
sudo usermod -a -G video $USER

# Or run with appropriate permissions
sudo ./panelkit --display-backend sdl_drm
```

**Wrong resolution**:
- SDL+DRM uses display's native resolution
- Application must adapt to actual dimensions

### Logging

Display backend selection and initialization is logged:
```
2025-05-23 02:42:53 INFO [panelkit] Display backend requested: sdl_drm
2025-05-23 02:42:53 INFO [panelkit] Auto-detected embedded environment, using SDL+DRM
2025-05-23 02:42:53 INFO [panelkit] Display backend created: SDL+DRM (1920x1080)
```

## Future Extensions

The display backend system is designed for extensibility:

1. **Wayland Backend**: Native Wayland without X11
2. **Direct FB**: Even more minimal without SDL
3. **Remote Display**: Network-based rendering
4. **Hardware Acceleration**: GPU-specific backends

To add a new backend:
1. Implement the `DisplayBackend` interface
2. Add to `display_backend_create()` switch
3. Update auto-detection logic if needed