# PanelKit Architecture

This document describes the comprehensive architecture of PanelKit SDL, a touch-optimized UI application designed for embedded Linux devices with minimal dependencies.

## System Overview

PanelKit follows a modular, layered architecture with clean separation of concerns and abstractions that enable both cross-platform development and embedded deployment.

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │   UI System     │  │ Gesture Engine  │  │  API Client     │  │
│  │ - Pages         │  │ - State Machine │  │ - HTTP/JSON     │  │
│  │ - Rendering     │  │ - Touch/Mouse   │  │ - Threading     │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Abstraction Layer                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │Display Backend  │  │ Input Handler   │  │   Core Utils    │  │
│  │ - SDL+DRM       │  │ - Evdev Source  │  │ - Logger        │  │
│  │ - Standard SDL  │  │ - SDL Source    │  │ - Build Info    │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Platform Layer                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │      SDL2       │  │  Linux Kernel   │  │   System Libs   │  │
│  │ - Video/Input   │  │ - DRM/evdev     │  │ - libc/libdrm   │  │
│  │ - Cross-platform│  │ - Device Access │  │ - Minimal deps  │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Core Architectural Principles

1. **Abstraction Over Implementation**: Clean interfaces allow swapping implementations without affecting higher layers
2. **Unified Input Handling**: Single code path for touch and mouse input ensures consistency
3. **Minimal Dependencies**: Embedded targets require only essential libraries (libdrm + system libs)
4. **Cross-Platform Development**: Same codebase runs on macOS/Linux for development and ARM64 embedded targets
5. **Production-Ready Deployment**: Systemd integration with comprehensive logging and error handling

## Module Architecture

### 1. Display Backend Abstraction (`src/display/`)

Provides a unified interface over different rendering approaches:

```c
typedef struct DisplayBackend {
    DisplayBackendType type;
    const char* name;
    void* impl;                    // Private implementation data
    SDL_Window* window;           // May be NULL for some backends
    SDL_Renderer* renderer;       // SDL renderer handle
    int actual_width, actual_height;
    
    // Operations
    void (*present)(DisplayBackend* backend);
    void (*cleanup)(DisplayBackend* backend);
    bool (*set_vsync)(DisplayBackend* backend, bool enable);
    bool (*set_fullscreen)(DisplayBackend* backend, bool enable);
} DisplayBackend;
```

**Implementations:**
- **`backend_sdl.c`**: Standard SDL with window manager support (development)
- **`backend_sdl_drm.c`**: Direct DRM framebuffer rendering (embedded production)

**Backend Selection Logic:**
```c
// Auto-detection for embedded vs development
if (/* embedded environment detected */) {
    backend_type = DISPLAY_BACKEND_SDL_DRM;  // Minimal dependencies
} else {
    backend_type = DISPLAY_BACKEND_SDL;      // Full SDL stack
}
```

### 2. Input Handler Abstraction (`src/input/`)

Provides pluggable input sources with unified event delivery:

```c
typedef struct InputHandler {
    InputSourceType source_type;
    void* source_impl;
    InputConfig config;
    bool is_running;
    
    // Statistics
    uint64_t total_events;
    uint64_t touch_events;
    uint64_t mouse_events;
    uint64_t keyboard_events;
} InputHandler;
```

**Input Sources:**
- **`input_source_evdev.c`**: Direct Linux input device access (production)
- **`input_source_sdl.c`**: SDL native input handling (development)
- **`input_source_mock.c`**: Simulated input for testing

**Unified Event Flow:**
```
Hardware Input → Input Source → Normalized Events → Application Handler → UI Actions
```

**Event Injection Pattern:**
```c
// All input sources inject events into SDL event queue
SDL_Event touch_event = {
    .type = SDL_FINGERDOWN,
    .tfinger = {
        .x = normalized_x,      // 0.0 - 1.0
        .y = normalized_y,      // 0.0 - 1.0
        .fingerId = touch_id,
        .pressure = pressure
    }
};
SDL_PushEvent(&touch_event);
```

### 3. Unified Touch/Mouse Handling (`src/app.c`)

All input events (mouse and touch) are routed through unified handlers:

```c
// SDL event loop routes both input types to same handlers
case SDL_MOUSEBUTTONDOWN:
    handle_touch_down(e.button.x, e.button.y, "mouse");
    break;

case SDL_FINGERDOWN:
    {
        int touch_x = (int)(e.tfinger.x * actual_width);
        int touch_y = (int)(e.tfinger.y * actual_height);
        handle_touch_down(touch_x, touch_y, "touch");
    }
    break;
```

**Gesture State Machine:**
```
NONE → POTENTIAL → {CLICK, DRAG_VERT, DRAG_HORZ, HOLD}
  ↑                              ↓
  └──────────── END ←────────────┘
```

### 4. Core Utilities (`src/core/`)

**Logger System (`logger.c`):**
- Structured logging with categories (INFO, DEBUG, ERROR)
- File-based output for remote debugging
- System introspection and state logging

**Build Information (`build_info.c`):**
- Compile-time build metadata
- Library version introspection
- Platform and configuration details

**SDL Include Management (`sdl_includes.h`):**
- Centralized header management for cross-platform builds
- Platform-specific include path handling
- Solves macOS Homebrew vs Linux package differences

### 5. API Integration (`src/api_functions.c`)

**Threading Model:**
```c
// Background API fetching with main thread safety
pthread_create(&api_thread, NULL, api_worker_thread, &api_data);
pthread_mutex_lock(&api_data.mutex);
// Access shared data safely
pthread_mutex_unlock(&api_data.mutex);
```

## Cross-Platform Build System

### Host vs Target Separation

The build system maintains separate configurations to avoid conflicts:

```
build/
├── host/          # Development builds (macOS/Linux)
│   ├── CMakeCache.txt
│   └── panelkit
└── target/        # ARM64 cross-compilation
    ├── CMakeCache.txt
    └── panelkit
```

### SDL Header Management

Different platforms have different SDL installation patterns:

```c
#ifdef EMBEDDED_BUILD
    #include <SDL2/SDL.h>         // Cross-compilation prefix
#elif defined(__APPLE__)
    #include <SDL.h>              // Homebrew with include/SDL2/ in path
#elif defined(__linux__)
    #include <SDL.h>              // Package manager installation
#else
    #include <SDL2/SDL.h>         // Fallback
#endif
```

**CMake Include Path Logic:**
```cmake
if(APPLE AND NOT EMBEDDED_BUILD)
    target_include_directories(target PUBLIC ${SDL2_INCLUDE_DIRS}/SDL2)
else()
    target_include_directories(target PUBLIC ${SDL2_INCLUDE_DIRS})
endif()
```

### Cross-Compilation via Docker

ARM64 builds use containerized cross-compilation:

```dockerfile
# Dockerfile.target provides consistent ARM64 build environment
FROM debian:bookworm
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    crossbuild-essential-arm64 \
    # ... other dependencies

# Custom SDL2 build from source for minimal dependencies
RUN cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/build/arm64-toolchain.cmake \
    -DSDL_SHARED=OFF -DSDL_STATIC=ON \
    # ... SDL configuration for embedded use
```

## Input System Deep Dive

### Device Detection and Selection

```c
// Automatic input source selection based on display backend
if (display_backend->type == DISPLAY_BACKEND_SDL_DRM) {
    // Embedded environment: use evdev for direct hardware access
    input_config.source_type = INPUT_SOURCE_LINUX_EVDEV;
    input_config.auto_detect_devices = true;
} else {
    // Development environment: use SDL native input
    input_config.source_type = INPUT_SOURCE_SDL_NATIVE;
}
```

### Touch Device Auto-Detection

```c
// Scan /dev/input/event* for touch capabilities
for (int i = 0; i < 32; i++) {
    snprintf(device_path, sizeof(device_path), "/dev/input/event%d", i);
    
    // Check for touch capabilities using ioctl
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) >= 0) {
        if (test_bit(ABS_MT_POSITION_X, abs_bits) && 
            test_bit(ABS_MT_POSITION_Y, abs_bits)) {
            // Multi-touch device found
            log_info("Found touch device: %s", device_path);
            return device_path;
        }
    }
}
```

### Event Coordinate Normalization

```c
// Normalize hardware coordinates to 0.0-1.0 range
float normalized_x = (float)(raw_x - touch_min_x) / (touch_max_x - touch_min_x);
float normalized_y = (float)(raw_y - touch_min_y) / (touch_max_y - touch_min_y);

// Clamp to valid range
normalized_x = fmaxf(0.0f, fminf(1.0f, normalized_x));
normalized_y = fmaxf(0.0f, fminf(1.0f, normalized_y));
```

## Display System Deep Dive

### SDL+DRM Integration

The SDL+DRM backend combines SDL's rendering capabilities with direct DRM framebuffer access:

```c
// SDL setup with offscreen driver (no window manager needed)
SDL_SetVideoDriver("offscreen");
SDL_Window* window = SDL_CreateWindow("PanelKit", 0, 0, width, height, SDL_WINDOW_HIDDEN);
SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

// DRM setup for hardware display
int drm_fd = open("/dev/dri/card1", O_RDWR);
drmModeGetResources(drm_fd);
// ... DRM configuration for connected display

// Render loop: SDL renders to memory, DRM displays the buffer
SDL_RenderPresent(renderer);                    // Render to SDL surface
copy_surface_to_drm_buffer(renderer, drm_buffer); // Copy to DRM buffer
drmModeSetCrtc(drm_fd, crtc_id, fb_id, ...);   // Display on hardware
```

**Benefits:**
- No X11/Wayland dependency (minimal system)
- Hardware-synchronized presentation (no tearing)
- Static linking possible (< 200KB runtime dependencies)

### Display Backend Auto-Selection

```c
bool is_embedded_environment() {
    // Check for embedded indicators
    if (access("/dev/dri/card1", F_OK) == 0 &&          // DRM device available
        getenv("DISPLAY") == NULL &&                     // No X11 display
        getenv("WAYLAND_DISPLAY") == NULL) {            // No Wayland display
        return true;
    }
    return false;
}
```

## Testing Architecture

### Systematic Testing Approach

The test suite follows a layered validation approach:

```
Hardware Layer Tests:
├── test_touch_raw.c          # Direct evdev testing (no SDL)
├── test_drm_basic.c          # Direct DRM testing (no SDL)
└── test_device_detection.c   # Hardware enumeration

Integration Tests:
├── test_sdl_drm_touch.c      # Complete SDL+DRM+Touch solution
├── test_input_handler.c      # Input abstraction testing
└── test_display_backend.c    # Display abstraction testing

System Tests:
├── test_unified_input.c      # Unified touch/mouse handling
├── test_gesture_engine.c     # Gesture recognition
└── test_api_integration.c    # API client functionality
```

### Test Deployment System

```bash
# Deploy source code to target for native compilation
make deploy-input-source

# Build tests on target with native tools
ssh target 'cd /tmp/panelkit-tests && make build-tests'

# Run comprehensive test suite
ssh target 'cd /tmp/panelkit-tests && ./run_all_tests.sh'
```

## Deployment Architecture

### Service-Based Deployment

```
Development Machine         Target Device
├── make target     →      ├── /tmp/panelkit/        (deployment)
├── make deploy     →      ├── make setup            (system config)
└── Cross-compilation      ├── make install          (service install)
                          └── systemctl start panelkit
```

**Systemd Service Design:**
```ini
[Unit]
Description=PanelKit Touch UI Application
After=graphical.target

[Service]
Type=simple
ExecStart=/usr/local/bin/panelkit
Restart=always
RestartSec=5
User=root
Environment=SDL_VIDEODRIVER=offscreen
StandardOutput=file:/var/log/panelkit/panelkit.log
StandardError=file:/var/log/panelkit/error.log

[Install]
WantedBy=graphical.target
```

### Zero-Downtime Updates

```bash
# 1. Deploy new version to staging location
make deploy TARGET_PATH=/tmp/panelkit-new

# 2. Validate new version
ssh target 'cd /tmp/panelkit-new && ./panelkit --version'

# 3. Atomic service update
ssh target 'sudo systemctl stop panelkit && \
           sudo cp /tmp/panelkit-new/panelkit /usr/local/bin/ && \
           sudo systemctl start panelkit'
```

## Performance Characteristics

### Memory Usage
- **Base Application**: ~2-4 MB RAM
- **SDL Surfaces**: ~1.2 MB for 480x640x32bpp
- **Input Buffers**: ~4 KB for event queues
- **API Cache**: ~1 KB for user data

### CPU Usage
- **Idle**: <1% CPU (60 FPS with minimal redraws)
- **Touch Events**: ~2-5% CPU per gesture
- **Page Transitions**: ~10-15% CPU during animation
- **API Requests**: ~1-2% CPU for background HTTP

### Storage Requirements
- **Binary Size**: ~500KB (statically linked)
- **Runtime Dependencies**: libdrm (~200KB), system libs
- **Logs**: ~1MB/day typical usage

## Error Handling Strategy

### Graceful Degradation

```c
// Display backend fallback
DisplayBackend* backend = display_backend_create(&config);
if (!backend) {
    log_warn("Preferred backend failed, trying fallback");
    config.backend_type = DISPLAY_BACKEND_SDL;
    backend = display_backend_create(&config);
}

// Input source fallback
if (!input_handler_start(input_handler)) {
    log_warn("Evdev input failed, falling back to SDL");
    input_config.source_type = INPUT_SOURCE_SDL_NATIVE;
    input_handler = input_handler_create(&input_config);
}
```

### Error Recovery

```c
// Service auto-restart on critical failures
if (sdl_init_failed || display_backend_failed) {
    log_error("Critical system failure, exiting for systemd restart");
    cleanup_and_exit(EXIT_FAILURE);
}

// Non-critical error handling
if (api_request_failed) {
    log_warn("API request failed, continuing with cached data");
    continue_with_cached_data();
}
```

## Security Considerations

### Input Device Access
- Application runs as root for `/dev/input/event*` access
- Minimal attack surface (no network services)
- Read-only access to input devices

### Network Security
- HTTPS-only API communication
- No incoming network connections
- Minimal curl configuration

### System Integration
- Systemd service isolation
- No shell command execution
- Controlled file system access

## Event-Driven Architecture

### State Management System

The state store provides thread-safe, type-aware storage with compound keys:

```c
// Store data with compound keys: "type_name:id"
state_store_set(store, "weather_temperature", "New York", 
               &weather_data, sizeof(weather_data));

// Retrieve with type safety
WeatherData* data = state_store_get(store, "weather_temperature", 
                                  "New York", &size, &timestamp);

// Wildcard iteration
state_store_iterate_wildcard(store, "weather_*:*", callback, context);
```

### Event System

Decoupled publish/subscribe system for loose coupling between components:

```c
// Subscribe to events
event_subscribe(event_system, "weather.temperature", 
               handle_weather_update, context);

// Publish events
WeatherData weather = {.temperature = 72.5f, .location = "NYC"};
event_publish(event_system, "weather.temperature", 
             &weather, sizeof(weather));
```

### Event-Aware UI Components

UI elements can subscribe to events and update automatically:

```c
// Proof of concept event button
EventButtonPOC* button = event_button_poc_create(x, y, w, h, 
                                               "Weather", 
                                               "weather.temperature");
event_button_poc_subscribe(button, event_system, state_store);

// Button automatically updates when weather events are published
```

## Configuration System

### YAML-Based Configuration

Hierarchical configuration with multiple sources:

```yaml
# System configuration (read-only)
display:
  brightness: 80
  timeout: 300

# User configuration
api:
  services:
    - id: weather
      host: api.weather.com
      endpoints:
        - name: current
          path: /current/{location}
          method: GET
```

### Configuration Loading Order

1. **System Config**: `/etc/panelkit/config.yaml` (defaults)
2. **User Config**: `~/.config/panelkit/config.yaml` (overrides)
3. **Local Config**: `./config.yaml` (development)
4. **CLI Arguments**: `--config key=value` (highest priority)

## Future Architecture Extensions

### Widget Interface Pattern (Phase 3)

```c
// Base widget interface with event integration
typedef struct Widget {
    char id[64];
    WidgetType type;
    SDL_Rect bounds;
    
    // Event integration
    char** subscribed_events;
    size_t num_subscriptions;
    
    // Operations
    void (*render)(Widget* self, SDL_Renderer* renderer);
    void (*handle_event)(Widget* self, const SDL_Event* event);
    void (*handle_data_event)(Widget* self, const char* event_name,
                             const void* data, size_t size);
    void (*destroy)(Widget* self);
} Widget;

// Concrete widget example
typedef struct {
    Widget base;
    float current_temperature;
    char location[64];
} WeatherWidget;
```

### Plugin System
```c
// Extensible input source registration
typedef struct {
    const char* name;
    InputSource* (*create)(const InputConfig* config);
    void (*destroy)(InputSource* source);
} InputSourcePlugin;

register_input_plugin(&evdev_plugin);
register_input_plugin(&custom_plugin);
```

## Development Workflow

### Rapid Iteration Cycle
1. **Local Development**: `make host && ./build/host/panelkit`
2. **Cross-Compilation**: `make target` (validates ARM64 compatibility)
3. **Deployment**: `make deploy` (copies to target device)
4. **Testing**: `ssh target 'cd /tmp/panelkit && ./panelkit'`
5. **Service Integration**: `ssh target 'cd /tmp/panelkit && make install'`

### Debug Workflow
1. **Touch Issues**: `./scripts/debug_touch.sh` (comprehensive diagnosis)
2. **Display Issues**: Check DRM device access and permissions
3. **Build Issues**: Verify Docker and cross-compilation environment
4. **Runtime Issues**: Review structured logs with context

This architecture provides a solid foundation for embedded UI applications while maintaining development velocity and deployment reliability.