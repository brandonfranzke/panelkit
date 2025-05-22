# PanelKit SDL Display Issue - Progress Report

## Context & Goal
We have a C/SDL touch panel application for Raspberry Pi CM5 that successfully compiles, deploys, and runs as a systemd service. The application logic works correctly (API calls, logging, rendering to software renderer), but **no graphics appear on the physical display**. The goal is to get actual visual output on the Raspberry Pi's connected display.

## Current Status
- ✅ **Application runs successfully** - no crashes, proper service startup
- ✅ **SDL initializes** - creates windows, renderers, processes input
- ✅ **Software rendering works** - application draws to memory buffers
- ❌ **No display output** - screen remains blank despite successful rendering

## Architecture Overview
- **Target**: Raspberry Pi CM5 (ARM64) running Raspberry Pi OS
- **Build**: Cross-compiled in Docker (Debian Bookworm with ARM64 toolchain)
- **SDL**: Custom-built SDL 2.30.x with static linking for embedded deployment
- **Display**: Connected via standard display interface (exact specs TBD)

## Core Problem: Missing Video Drivers

### Observed Issue
```bash
# Current SDL driver enumeration:
Available drivers: 3
  0: offscreen  # Memory-only rendering
  1: dummy      # No-op driver  
  2: evdev      # Input driver, not video

# Expected but missing:
- KMSDRM (modern Linux framebuffer via DRM/KMS)
- fbdev (legacy framebuffer)
```

### Root Cause Analysis
Through systematic testing, we identified that **SDL framebuffer drivers are not being compiled** into our custom SDL build, despite configuration appearing correct:

1. **SDL Configuration appears correct**:
   ```
   SDL_KMSDRM:BOOL=ON
   PKG_KMSDRM_FOUND:INTERNAL=1
   All DRM/GBM/EGL libraries detected
   ```

2. **But compilation fails silently**:
   - Verbose build shows no KMSDRM source files compiled
   - `strings libSDL2.a | grep kmsdrm` returns nothing
   - Driver enumeration only shows offscreen/dummy/evdev

3. **Current hypothesis**: Cross-compilation EGL header issue
   - SDL's KMSDRM requires: `PKG_KMSDRM_FOUND AND HAVE_OPENGL_EGL`
   - EGL detection likely failing in cross-compilation environment
   - Headers exist in `/usr/include/EGL/` but not in ARM64 cross-compilation paths

## Testing Methodology

### Minimal Test App Approach
Created isolated test applications in `test_minimal/` directory to avoid disrupting main application:

**test_display.c**: Basic SDL window with colored rectangle
**test_forced_driver.c**: Systematically tries forcing different video drivers via environment variables

**Benefits**:
- Fast iteration (minimal build time)
- Clear debug output
- Isolated from main application complexity
- Easy to create variants testing different approaches

### Key Test Results
```bash
# Forcing different drivers via SDL_VIDEODRIVER:
fbdev: "fbdev not available"
KMSDRM: "KMSDRM not available" 
offscreen: ✅ Works (but no display output)
```

This confirms drivers are completely missing from SDL build, not just enumeration issues.

## Current Investigation Plan

### Immediate Next Steps
1. **Fix EGL cross-compilation**: Add EGL headers to ARM64 cross-compilation paths
2. **Verify HAVE_OPENGL_EGL**: Ensure EGL detection passes during SDL build
3. **Confirm KMSDRM compilation**: Check that KMSDRM sources actually get compiled
4. **Test framebuffer output**: Verify working drivers produce display output

### Docker Build Changes Made
- Added comprehensive DRM/GBM/EGL dependencies
- Added cross-compilation EGL header copying
- Enhanced build debugging and verification
- Explicit library path configuration for cross-compilation

### Fallback Approaches
If KMSDRM continues to fail:
1. **Legacy fbdev**: Focus on getting basic framebuffer driver working
2. **Raspberry Pi specific driver**: Try SDL_RPI driver with VideoCore libraries
3. **System configuration**: Investigate Pi boot config for KMS/DRM mode
4. **Alternative SDL build**: Try different SDL version or build approach

## Technical Details

### Build Environment
- **Container**: Debian Bookworm with crossbuild-essential-arm64
- **Toolchain**: aarch64-linux-gnu-gcc
- **SDL Build**: From source with custom configuration for embedded use
- **Dependencies**: DRM, GBM, EGL, udev libraries for framebuffer support

### SDL Configuration
```cmake
-DSDL_KMSDRM=ON
-DSDL_FBDEV=ON  
-DSDL_OPENGL=ON
-DSDL_OPENGLES=ON
-DSDL_RENDER_SOFTWARE=ON
# Disabled: X11, Wayland, audio, joystick, etc.
```

### Key Files
- **Main app**: `src/app.c` - Full application with logging, API calls, UI
- **Test apps**: `test_minimal/test_display.c`, `test_minimal/test_forced_driver.c`
- **Build**: `Dockerfile.target` - Cross-compilation environment and SDL build
- **Deploy**: `deploy/panelkit.service` - Systemd service configuration

## Questions for Next Session
1. Should we continue with EGL header fix approach or try different strategy?
2. Are there Raspberry Pi specific display configuration requirements we're missing?
3. Should we try a completely different SDL build approach (package manager vs source)?
4. Is there value in testing with simpler graphics output (direct framebuffer writes)?

## Success Criteria
- SDL enumerates KMSDRM or fbdev drivers
- `test_forced_driver` successfully initializes framebuffer driver
- Colored rectangle appears on physical display
- Main application displays UI on screen

---

**Current Status**: Ready to test EGL header fix and verify KMSDRM compilation. If this doesn't work, we have clear fallback strategies and a solid testing framework for rapid iteration.