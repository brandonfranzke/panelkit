# PanelKit with LVGL

This document outlines the approach for implementing PanelKit using LVGL (Light and Versatile Graphics Library).

## Project Objectives

PanelKit is designed as a touch-centric UI application targeting both development machines (macOS) and embedded Linux devices (Raspberry Pi). The primary goals are:

1. Create a responsive, touch-first interface for embedded devices
2. Simplify cross-compilation between development and target environments
3. Optimize UI elements for touchscreen interaction
4. Establish a clean, maintainable codebase with minimal dependencies
5. Enable seamless deployment to target hardware

## Why LVGL?

LVGL is chosen for this project because:
- Extremely lightweight core (~100KB) with minimal dependencies
- Mature, well-tested codebase optimized for embedded systems
- Native touch input and gesture handling
- Direct framebuffer access without requiring X11 or Wayland
- MIT license with no commercial restrictions
- Built-in support for automatic layout and content sizing
- Efficient memory usage for resource-constrained devices

## Development Environment

### Dependencies

LVGL has minimal dependencies:
- C compiler (GCC/Clang)
- CMake (for build system)
- For framebuffer access: Linux framebuffer headers
- Optional: SDL2 for development/simulation on desktop

For a full desktop development environment:
```
# Ubuntu/Debian
sudo apt-get install build-essential cmake libsdl2-dev

# macOS
brew install cmake sdl2
```

### Cross-Compilation with Docker

A Docker-based approach provides a clean, reproducible build environment:

```dockerfile
FROM ubuntu:22.04

# Install build tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsdl2-dev \
    crossbuild-essential-arm64 \
    pkg-config

# Clone LVGL
RUN git clone https://github.com/lvgl/lvgl.git /lvgl

# Set up working directory
WORKDIR /project
```

### Build System

CMake is recommended for the build system, with separate targets for:
- Desktop development build (using SDL2 renderer)
- Embedded target cross-compilation (using framebuffer)

## UI Implementation

### Button Design Requirements
- Buttons sized at 50% of viewport width
- Button height at 2/3 of viewport height
- Center-aligned in layout
- Clear visual distinction and adequate touch targets

### Layout Considerations
- Portrait orientation (640x480)
- Relative sizing (%, fractions) for responsive layout
- Scrollable areas with proper touch event handling
- Customizable background without artifacts

### LVGL-Specific Components

1. **lv_obj_t as base container**
   - Create a main container for the application
   - Set layout to LV_LAYOUT_FLEX with vertical direction

2. **lv_btn and lv_label for buttons**
   - Create styled buttons with appropriate sizing
   - Add labels for button text
   - Configure click events

3. **lv_obj with flex layout and lv_scroll options**
   - Create a scrollable container
   - Add buttons as children to this container
   - Enable touch scrolling with momentum

4. **lv_style for theming**
   - Define consistent styles for UI elements
   - Support background color customization

## Platform Adaptation

### Development Environment (macOS)
- Use SDL2 renderer for development and testing
- Same codebase with conditional compilation

### Target Environment (Raspberry Pi)
- Use Linux framebuffer (fbdev) for direct rendering
- Optimized for Raspberry Pi CM5 with touchscreen
- Portrait orientation (640x480)
- Auto-start via systemd service

## Critical Implementation Components

### Framebuffer Access
```c
lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.draw_buf = &disp_buf;
disp_drv.flush_cb = fbdev_flush;  // Framebuffer flush callback
disp_drv.hor_res = 640;
disp_drv.ver_res = 480;
lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
```

### Touch Input 
```c
lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = touch_read;  // Touch input callback
lv_indev_t *touch_indev = lv_indev_drv_register(&indev_drv);
```

### Automatic Layout
```c
lv_obj_t *cont = lv_obj_create(lv_scr_act());
lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
```

### Scrollable Container
```c
lv_obj_t *scroll_cont = lv_obj_create(cont);
lv_obj_set_size(scroll_cont, LV_PCT(100), LV_PCT(100));
lv_obj_set_flex_flow(scroll_cont, LV_FLEX_FLOW_COLUMN);
lv_obj_set_style_pad_all(scroll_cont, 20, 0);
lv_obj_set_style_gap(scroll_cont, 20, 0);
lv_obj_set_scrollbar_mode(scroll_cont, LV_SCROLLBAR_MODE_AUTO);
lv_obj_set_scroll_dir(scroll_cont, LV_DIR_VER);
lv_obj_set_scroll_snap_y(scroll_cont, LV_SCROLL_SNAP_NONE);
```

## Deployment Process

1. Cross-compile the application using Docker
2. Deploy binary to Raspberry Pi
3. Configure systemd service for auto-start
4. Set up appropriate permissions for framebuffer access

## Known LVGL Considerations

1. **Animation Performance**: Memory constraints can affect complex animations
2. **Unicode Fonts**: Large fonts with many characters increase binary size
3. **Screen Rotation**: May need manual management depending on display driver
4. **Framebuffer Permissions**: User needs access to /dev/fb0 and /dev/input devices

## Next Steps

1. Set up Docker build environment with cross-compilation support
2. Create basic application structure with main display and input handling
3. Implement UI components according to specifications
4. Test deployment on target hardware
5. Optimize for performance and responsiveness