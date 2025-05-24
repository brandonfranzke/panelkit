# PanelKit Input System

This document describes the comprehensive input handling architecture in PanelKit, including the unified touch/mouse system, input abstraction layer, and debugging methodology.

## Overview

The PanelKit input system is designed around a core principle: **unified input handling**. Both touch and mouse events flow through the same gesture recognition code path, ensuring consistent behavior between development (mouse) and production (touch) environments.

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Hardware Input  │    │ Input Sources   │    │ Unified Handler │
│ - Touch Screen  │───►│ - Evdev Source  │───►│ - Touch Events  │
│ - Mouse/Trackpad│    │ - SDL Source    │    │ - Gesture Engine│
│ - Keyboard      │    │ - Mock Source   │    │ - State Machine │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                       │
                                                       ▼
                       ┌─────────────────────────────────────────┐
                       │           Application Layer            │
                       │ - UI Actions  - Page Navigation        │
                       │ - Button Taps - Content Scrolling      │
                       │ - Swipe Gestures - Visual Feedback     │
                       └─────────────────────────────────────────┘
```

## Architecture Components

### 1. Input Handler (`src/input/input_handler.h`)

The central coordinator that manages input sources and provides a unified interface:

```c
typedef struct InputHandler {
    InputSourceType source_type;     // Current input source type
    void* source_impl;               // Implementation-specific data
    InputConfig config;              // Configuration parameters
    bool is_running;                 // Handler state
    
    // Statistics for debugging
    uint64_t total_events;
    uint64_t touch_events;
    uint64_t mouse_events;
    uint64_t keyboard_events;
} InputHandler;
```

**Key Functions:**
- `InputHandler* input_handler_create(const InputConfig* config)` - Create handler with configuration
- `bool input_handler_start(InputHandler* handler)` - Start event processing
- `void input_handler_stop(InputHandler* handler)` - Stop event processing
- `void input_handler_destroy(InputHandler* handler)` - Cleanup and destroy

### 2. Input Sources

#### A. Evdev Source (`src/input/input_source_evdev.c`)

Direct Linux input device access for production embedded systems:

**Features:**
- Auto-detection of touch devices in `/dev/input/event*`
- Multi-touch protocol support (MT slots, tracking IDs)
- Coordinate normalization to 0.0-1.0 range
- Pressure and touch size support
- Non-blocking I/O with background thread

**Device Detection:**
```c
// Scan for devices with touch capabilities
for (int i = 0; i < 32; i++) {
    snprintf(device_path, sizeof(device_path), "/dev/input/event%d", i);
    
    // Check capabilities using ioctl
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) >= 0) {
        if (test_bit(ABS_MT_POSITION_X, abs_bits) && 
            test_bit(ABS_MT_POSITION_Y, abs_bits)) {
            // Multi-touch device found
            return device_path;
        }
    }
}
```

**Event Processing:**
```c
// Read raw input events
struct input_event ev;
while (read(device_fd, &ev, sizeof(ev)) == sizeof(ev)) {
    switch (ev.type) {
        case EV_ABS:
            if (ev.code == ABS_MT_TRACKING_ID) {
                if (ev.value == -1) {
                    // Touch up
                    inject_touch_up_event(slot);
                } else {
                    // Touch down
                    inject_touch_down_event(slot, ev.value);
                }
            }
            break;
    }
}
```

#### B. SDL Source (`src/input/input_source_sdl.c`)

Standard SDL input handling for development environments:

**Features:**
- Native SDL event processing
- Mouse and keyboard event handling
- Cross-platform compatibility
- Immediate event delivery (no threading)

**Event Forwarding:**
```c
// SDL events are already in the correct format
// Just ensure they reach the application event loop
SDL_Event event;
while (SDL_PollEvent(&event)) {
    // Events automatically go to SDL's main event queue
    // No additional processing needed
}
```

#### C. Mock Source (`src/input/input_source_mock.c`)

Simulated input for testing and development:

**Features:**
- Programmatic event generation
- Gesture pattern simulation
- Performance testing scenarios
- Automated UI testing support

### 3. Unified Touch Handling (`src/app.c`)

The application layer receives all input through unified handlers:

```c
// Both mouse and touch events use the same handlers
void handle_touch_down(int x, int y, const char* source) {
    if (target_page == -1) {
        int button_index = get_button_at_position(x, y, pages[current_page].scroll_position);
        begin_gesture(x, y, button_index);
        gesture_page = current_page;
        log_debug("Touch DOWN from %s at (%d,%d), button=%d", source, x, y, button_index);
    }
}

void handle_touch_up(int x, int y, const char* source) {
    end_gesture(x, y);
    log_debug("Touch UP from %s at (%d,%d)", source, x, y);
}

void handle_touch_motion(int x, int y, const char* source) {
    update_gesture(x, y);
}
```

**Event Routing:**
```c
// SDL event loop routes all input to unified handlers
switch (e.type) {
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
}
```

## Gesture Recognition Engine

### State Machine

The gesture engine uses a finite state machine to classify input patterns:

```
┌─────────────┐
│ GESTURE_NONE│
└──────┬──────┘
       │ Touch Down
       ▼
┌─────────────────┐
│GESTURE_POTENTIAL│
└─────────┬───────┘
          │
          ▼
    ┌─────────────┐ Time > Hold ┌──────────────┐
    │   Analyze   │────────────►│GESTURE_HOLD  │
    │ Movement &  │             └──────────────┘
    │   Timing    │
    └─────┬───────┘
          │
          ▼
    ┌─────────────┐ Movement ┌─────────────────┐
    │  Movement   │◄────────►│    Direction    │
    │ Detection   │          │   Analysis      │
    └─────┬───────┘          └─────┬───────────┘
          │                        │
          ▼                        ▼
┌─────────────────┐        ┌─────────────────┐
│ GESTURE_CLICK   │        │GESTURE_DRAG_*   │
│ (Quick tap)     │        │(Scroll/Swipe)   │
└─────────────────┘        └─────────────────┘
```

### Thresholds and Configuration

```c
// Gesture detection thresholds
#define CLICK_TIMEOUT_MS 300         // Max time for click vs hold
#define DRAG_THRESHOLD_PX 10         // Min movement for drag
#define HOLD_THRESHOLD_MS 500        // Min time for hold gesture
#define PAGE_SWIPE_THRESHOLD_PX 100  // Min horizontal movement for page change
```

### Gesture Classification Logic

```c
void update_gesture(int x, int y) {
    if (current_gesture == GESTURE_POTENTIAL) {
        int dx = abs(x - gesture_start_x);
        int dy = abs(y - gesture_start_y);
        
        if (dx > DRAG_THRESHOLD_PX || dy > DRAG_THRESHOLD_PX) {
            if (dx > dy * 1.5) {
                // Horizontal movement dominates - page swipe
                current_gesture = GESTURE_DRAG_HORZ;
                setup_page_transition();
            } else if (dy > dx * 1.5) {
                // Vertical movement dominates - content scroll
                current_gesture = GESTURE_DRAG_VERT;
            }
        }
    }
    
    // Update gesture state based on movement
    if (current_gesture == GESTURE_DRAG_HORZ) {
        update_page_transition(x - gesture_start_x);
    } else if (current_gesture == GESTURE_DRAG_VERT) {
        update_content_scroll(y - gesture_start_y);
    }
}
```

## Event Flow and Coordinate Systems

### Coordinate Normalization

Different input sources provide coordinates in different formats:

**Evdev Raw Coordinates:**
```c
// Hardware-specific coordinate ranges
struct input_absinfo abs_x, abs_y;
ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_x);
ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_y);

// Normalize to 0.0-1.0 range
float normalized_x = (float)(raw_x - abs_x.minimum) / (abs_x.maximum - abs_x.minimum);
float normalized_y = (float)(raw_y - abs_y.minimum) / (abs_y.maximum - abs_y.minimum);
```

**SDL Touch Events:**
```c
// SDL provides normalized coordinates (0.0-1.0)
case SDL_FINGERDOWN:
    // Convert to pixel coordinates for unified handling
    int pixel_x = (int)(e.tfinger.x * actual_width);
    int pixel_y = (int)(e.tfinger.y * actual_height);
```

**SDL Mouse Events:**
```c
// SDL provides pixel coordinates directly
case SDL_MOUSEBUTTONDOWN:
    // Already in pixel coordinates
    handle_touch_down(e.button.x, e.button.y, "mouse");
```

### Event Injection Pattern

All input sources inject events into SDL's event queue for unified processing:

```c
// Create SDL touch event
SDL_Event touch_event = {
    .type = SDL_FINGERDOWN,
    .tfinger = {
        .touchId = 0,
        .fingerId = tracking_id,
        .x = normalized_x,
        .y = normalized_y,
        .dx = 0.0f,
        .dy = 0.0f,
        .pressure = pressure
    }
};

// Inject into SDL event queue
if (SDL_PushEvent(&touch_event) < 0) {
    log_error("Failed to inject touch event: %s", SDL_GetError());
}
```

## Auto-Detection and Configuration

### Input Source Selection

The system automatically selects the appropriate input source based on the display backend:

```c
// Automatic input source selection
InputConfig input_config = {
    .source_type = INPUT_SOURCE_SDL_NATIVE,  // Default
    .device_path = NULL,                      // Auto-detect
    .auto_detect_devices = true,
    .enable_mouse_emulation = false
};

// Switch to evdev for embedded environments
if (display_backend->type == DISPLAY_BACKEND_SDL_DRM) {
    log_info("SDL+DRM backend detected, switching to evdev input source");
    input_config.source_type = INPUT_SOURCE_LINUX_EVDEV;
}
```

### Device Auto-Detection

```c
bool input_source_evdev_auto_detect_device(char* device_path, size_t path_size) {
    log_info("Scanning for touch devices in /dev/input...");
    
    for (int i = 0; i < 32; i++) {
        char path[256];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        
        // Get device name
        char name[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        
        // Check for touch capabilities
        unsigned long abs_bits[NBITS(ABS_MAX)] = {0};
        if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) >= 0) {
            if (test_bit(ABS_MT_POSITION_X, abs_bits) && 
                test_bit(ABS_MT_POSITION_Y, abs_bits)) {
                log_info("  Found touch device: %s - %s", path, name);
                strncpy(device_path, path, path_size - 1);
                close(fd);
                return true;
            }
        }
        
        close(fd);
    }
    
    return false;
}
```

## Debug and Introspection

### Input Debug System (`src/input/input_debug.c`)

Comprehensive debugging and introspection capabilities:

**Device Capabilities:**
```c
void input_debug_log_device_caps(const char* device_path) {
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) return;
    
    // Log device information
    char name[256] = {0};
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    log_info("Device: %s - %s", device_path, name);
    
    // Log supported event types
    unsigned long ev_bits[NBITS(EV_MAX)] = {0};
    ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
    
    if (test_bit(EV_ABS, ev_bits)) {
        log_info("  Supports absolute positioning");
        
        // Log absolute axis information
        for (int axis = 0; axis < ABS_MAX; axis++) {
            if (test_bit(axis, abs_bits)) {
                struct input_absinfo abs_info;
                ioctl(fd, EVIOCGABS(axis), &abs_info);
                log_info("    ABS_%s: min=%d max=%d", 
                        abs_axis_name(axis), abs_info.minimum, abs_info.maximum);
            }
        }
    }
    
    close(fd);
}
```

**SDL State Logging:**
```c
void input_debug_log_sdl_state(void) {
    log_info("=== SDL Input State ===");
    
    // Available video drivers
    int num_drivers = SDL_GetNumVideoDrivers();
    log_info("Available video drivers: %d", num_drivers);
    for (int i = 0; i < num_drivers; i++) {
        const char* driver = SDL_GetVideoDriver(i);
        const char* current = SDL_GetCurrentVideoDriver();
        log_info("  %s%s", driver, 
                (current && strcmp(driver, current) == 0) ? " (current)" : "");
    }
    
    // Touch device information
    int num_touch_devices = SDL_GetNumTouchDevices();
    log_info("Touch devices: %d", num_touch_devices);
    for (int i = 0; i < num_touch_devices; i++) {
        SDL_TouchID touch_id = SDL_GetTouchDevice(i);
        log_info("  Touch device %d: ID=%" PRId64, i, touch_id);
    }
}
```

### Comprehensive Debug Script (`scripts/debug_touch.sh`)

Interactive debugging tool for production environments:

**Features:**
- Permission checking (input/video groups)
- Touch device auto-detection
- Raw input event testing
- PanelKit integration testing
- Detailed troubleshooting guidance

**Usage:**
```bash
# On target device
./scripts/debug_touch.sh

# Interactive menu provides:
# 1) Check permissions
# 2) Find touch devices  
# 3) Test raw touch input
# 4) Run PanelKit with debug
# 5) Show summary
# 6) Run all tests
```

## Testing Methodology

### Layered Testing Approach

**Layer 1: Hardware Validation**
```bash
# Test raw Linux input without SDL
sudo ./test_touch_raw

# Expected output:
# [  0.123] TOUCH_DOWN (tracking_id: 23)
# [  0.134] ABS_MT_POSITION_X: 320 (normalized: 0.500)
# [  0.135] ABS_MT_POSITION_Y: 240 (normalized: 0.375)
# [  0.456] TOUCH_UP (tracking_id: 23)
```

**Layer 2: SDL Integration**
```bash
# Test SDL with direct touch input
./test_sdl_touch

# Expected output:
# SDL Touch device found: ID=0
# FINGERDOWN: id=23 x=0.500 y=0.375
# FINGERUP: id=23 x=0.500 y=0.375
```

**Layer 3: Complete System**
```bash
# Test complete SDL+DRM+Touch solution
./test_sdl_drm_touch

# Expected output:
# SDL+DRM backend initialized
# Touch device: /dev/input/event4
# TOUCH_DOWN injected: (320,240)
# TOUCH_UP injected: (320,240)
```

### Debugging Common Issues

**Issue: Touch Not Detected**
```bash
# Check device permissions
ls -la /dev/input/event*

# Check user groups
groups $USER | grep input

# Check device capabilities
sudo evtest /dev/input/event4
```

**Issue: Events Not Reaching Application**
```bash
# Check SDL video driver
echo $SDL_VIDEODRIVER

# Check for conflicting processes
sudo lsof /dev/input/event4

# Check application logs
tail -f /var/log/panelkit/panelkit.log | grep -i input
```

**Issue: Coordinate Mapping Problems**
```bash
# Test coordinate normalization
sudo ./test_touch_raw | grep -E "(ABS_MT_POSITION|normalized)"

# Check display resolution
./panelkit --display-backend sdl_drm 2>&1 | grep -i resolution
```

## Performance Considerations

### Threading Model

**Evdev Source:**
- Background thread for device reading
- Lock-free event injection to main thread
- Non-blocking I/O to prevent input lag

**SDL Source:**
- Main thread event processing
- Immediate event delivery
- No additional threading overhead

### Memory Usage

- **Event Buffers**: ~4KB per input source
- **Touch Point Tracking**: ~64 bytes per active touch
- **Gesture State**: ~256 bytes total
- **Debug Buffers**: ~1KB when debug logging enabled

### Latency Optimization

```c
// Minimize touch-to-screen latency
struct input_event events[64];
int count = read(device_fd, events, sizeof(events));

// Process events in batch
for (int i = 0; i < count / sizeof(struct input_event); i++) {
    process_event(&events[i]);
}

// Immediate event injection
SDL_PushEvent(&touch_event);  // No buffering
```

## Configuration Options

### Runtime Configuration

```c
typedef struct {
    InputSourceType source_type;      // INPUT_SOURCE_*
    const char* device_path;          // "/dev/input/eventX" or NULL for auto
    bool auto_detect_devices;         // Enable device auto-detection
    bool enable_mouse_emulation;      // Convert touch to mouse events
    int touch_pressure_threshold;     // Minimum pressure for touch
    float coordinate_scale_x;         // X coordinate scaling factor
    float coordinate_scale_y;         // Y coordinate scaling factor
} InputConfig;
```

### Environment Variables

```bash
# Force specific input source
export PANELKIT_INPUT_SOURCE=evdev

# Specify touch device
export PANELKIT_TOUCH_DEVICE=/dev/input/event4

# Enable debug logging
export PANELKIT_LOG_LEVEL=DEBUG

# SDL video driver selection
export SDL_VIDEODRIVER=offscreen
```

## Future Enhancements

### Multi-Touch Support

```c
// Track multiple simultaneous touches
typedef struct {
    int tracking_id;
    float x, y, pressure;
    bool active;
    uint32_t start_time;
} TouchPoint;

TouchPoint active_touches[10];  // Support up to 10 touches
```

### Gesture Recognition Extensions

```c
// Advanced gesture patterns
typedef enum {
    GESTURE_PINCH,          // Two-finger pinch/zoom
    GESTURE_ROTATE,         // Two-finger rotation
    GESTURE_THREE_FINGER_SWIPE,  // Three-finger gestures
    GESTURE_LONG_PRESS      // Extended hold with feedback
} AdvancedGestureType;
```

### Input Source Plugins

```c
// Pluggable input source architecture
typedef struct {
    const char* name;
    InputSource* (*create)(const InputConfig* config);
    void (*destroy)(InputSource* source);
    bool (*supports_device)(const char* device_path);
} InputSourcePlugin;

// Register custom input sources
register_input_plugin(&bluetooth_touch_plugin);
register_input_plugin(&network_input_plugin);
```

This comprehensive input system provides the foundation for reliable, responsive touch interaction while maintaining the flexibility to support various hardware configurations and development workflows.