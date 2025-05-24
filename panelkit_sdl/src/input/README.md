# Input Handler Abstraction

This module provides a clean abstraction for handling input across different backends, solving the specific problem of touch input not working with SDL's offscreen driver on embedded Linux systems.

## Architecture

The input system uses the Strategy pattern to support multiple input sources:

```
InputHandler (Manager)
    └── InputSource (Interface)
         ├── SDL Native (SDL's built-in events)
         ├── Linux evdev (Direct /dev/input reading)
         └── Mock (Testing/simulation)
```

## Problem Solved

When using SDL with the offscreen video driver (required for SDL+DRM display backend to avoid large Mesa dependencies), SDL's built-in input system doesn't work because the offscreen driver is designed for headless operation. This abstraction allows us to:

1. Use SDL's native input when available (desktop, KMSDRM driver)
2. Read touch input directly from Linux `/dev/input/event*` devices when needed
3. Inject the events back into SDL's event system for seamless integration

## Usage

### Basic Usage

```c
// Configure input
InputConfig config = {
    .source_type = INPUT_SOURCE_SDL_NATIVE,  // or INPUT_SOURCE_LINUX_EVDEV
    .device_path = NULL,                     // Auto-detect
    .auto_detect_devices = true,
    .enable_mouse_emulation = false
};

// Create and start handler
InputHandler* input = input_handler_create(&config);
input_handler_start(input);

// Events come through normal SDL_PollEvent()
SDL_Event event;
while (SDL_PollEvent(&event)) {
    // Handle SDL_FINGERDOWN, SDL_FINGERUP, SDL_FINGERMOTION, etc.
}

// Cleanup
input_handler_destroy(input);
```

### Automatic Backend Selection

```c
// Use evdev for SDL+DRM backend, SDL native otherwise
if (display_backend->type == DISPLAY_BACKEND_SDL_DRM) {
    config.source_type = INPUT_SOURCE_LINUX_EVDEV;
}
```

## Implementation Details

### SDL Native Source (`input_source_sdl.c`)
- Passthrough for SDL's built-in event system
- Works with X11, Wayland, KMSDRM backends
- No additional processing needed

### Linux evdev Source (`input_source_evdev.c`)
- Reads directly from `/dev/input/event*` devices
- Auto-detects touch devices (looks for ABS_MT_POSITION_X capability)
- Converts Linux input events to SDL touch events
- Handles multi-touch protocol (MT slots)
- Thread-safe event injection via `SDL_PushEvent()`

### Mock Source (`input_source_mock.c`)
- Generates test patterns (tap, swipe, pinch, circle)
- Useful for testing without hardware
- Programmable event sequences

## Touch Event Flow

1. **Hardware** → Linux kernel input driver
2. **Kernel** → `/dev/input/eventX` device file
3. **evdev source** → Reads input_event structures
4. **Conversion** → Transform to SDL touch events (normalized 0.0-1.0)
5. **Injection** → `SDL_PushEvent()` into SDL's event queue
6. **Application** → Normal `SDL_PollEvent()` processing

## Files

- `input_handler.h` - Public interface and types
- `input_handler.c` - Main handler implementation
- `input_source_sdl.c` - SDL native input (default)
- `input_source_evdev.c` - Linux direct input reading
- `input_source_mock.c` - Test/simulation input

## Testing

See `test/input/` for test programs:
- `test_input_handler.c` - Unit tests for each source
- `test_integrated_touch.c` - Full integration test with display backend

## Notes

- The evdev source requires root or input group permissions to read `/dev/input/event*`
- Touch coordinates are automatically normalized to 0.0-1.0 range
- Multi-touch is fully supported (up to 10 simultaneous touches)
- Thread-safe design allows input reading in background thread