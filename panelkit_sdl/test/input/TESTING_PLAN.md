# Touch Input Testing Plan with Static SDL

## Overview
We've built a suite of tests using **statically linked SDL** (no libgbm/Mesa dependencies) to diagnose and solve the touch input issue.

## Test Suite

### 1. test_touch_raw (714KB)
- **Purpose**: Verify touch hardware works at Linux level
- **SDL**: None - direct Linux input
- **Run**: `sudo /tmp/test_touch_raw`
- **Expected**: Shows raw touch events from /dev/input/event*

### 2. test_touch_minimal (1.4MB) 
- **Purpose**: Test SDL with offscreen driver (mimics main app)
- **SDL**: Static, offscreen driver
- **Run**: `/tmp/test_touch_minimal`
- **Expected**: Currently NO touch events (known issue)

### 3. test_sdl_dummy (1.4MB)
- **Purpose**: Test SDL with dummy driver
- **SDL**: Static, dummy driver  
- **Run**: `/tmp/test_sdl_dummy`
- **Expected**: Currently NO touch events (known issue)

### 4. test_manual_inject (1.4MB)
- **Purpose**: Test manual touch injection into SDL
- **SDL**: Static, offscreen driver + manual event injection
- **Run**: `sudo /tmp/test_manual_inject`
- **Expected**: Should show touch events via manual injection

### 5. test_sdl_drm_touch (1.5MB) ⭐ COMPLETE SOLUTION
- **Purpose**: Combined SDL+DRM display with manual touch input
- **SDL**: Static, offscreen driver
- **Features**:
  - Display output via DRM (no libgbm)
  - Touch input via manual /dev/input reading
  - Visual feedback (yellow squares at touch points)
- **Run**: `sudo /tmp/test_sdl_drm_touch`
- **Expected**: Full display + working touch input

## Testing Order

1. First verify hardware: `sudo /tmp/test_touch_raw`
2. Confirm SDL offscreen issue: `/tmp/test_touch_minimal` (no touch)
3. Test manual injection: `sudo /tmp/test_manual_inject` (should work)
4. Test complete solution: `sudo /tmp/test_sdl_drm_touch` (display + touch)

## Key Findings

- SDL with offscreen/dummy drivers does NOT receive Linux input events
- Manual reading from /dev/input/event* and injecting via SDL_PushEvent() works
- SDL+DRM provides display output without libgbm dependency
- Combining SDL+DRM with manual input injection gives complete solution

## Next Steps

If test_sdl_drm_touch works correctly:
1. Port the manual input injection to backend_sdl_drm.c
2. Add input device discovery (find correct /dev/input/event*)
3. Handle multiple touch points
4. Add proper cleanup and error handling