# Error Injection Points Plan

This document outlines strategic error injection points for testing error recovery and resilience in PanelKit.

## Overview

Error injection points allow controlled failure simulation during development and testing. These would be implemented as conditional compilation blocks that can trigger specific errors on demand.

## Implementation Pattern

```c
#ifdef DEBUG_ERROR_INJECTION
if (error_injection_should_fail("injection_point_name")) {
    pk_set_last_error_with_context(PK_ERROR_CODE, 
                                   "Injected error at %s", 
                                   "injection_point_name");
    return error_value;
}
#endif
```

## Proposed Injection Points

### Display Backend
1. **display_init_failure** - Simulate display initialization failure
   - Justification: Test fallback to different backends, resolution recovery
   - Location: display_backend_create()

2. **drm_master_loss** - Simulate loss of DRM master privileges  
   - Justification: Common when switching VTs on embedded
   - Location: backend_sdl_drm.c mode setting

3. **framebuffer_allocation_failure** - Simulate out of video memory
   - Justification: Test graceful degradation under memory pressure
   - Location: SDL_CreateRenderer calls

### Input System
4. **input_device_disappear** - Simulate /dev/input device removal
   - Justification: USB disconnect, permission changes
   - Location: input_source_evdev.c device read

5. **touch_calibration_corrupt** - Simulate invalid touch calibration data
   - Justification: Test fallback to default calibration
   - Location: Touch event transformation

6. **input_permission_denied** - Simulate permission errors
   - Justification: Common embedded issue with device nodes
   - Location: Device open operations

### Event System
7. **event_queue_overflow** - Force event queue full condition
   - Justification: Test event dropping strategies
   - Location: event_emit()

8. **event_subscriber_crash** - Simulate subscriber callback failure
   - Justification: Test isolation of failing subscribers
   - Location: Event dispatch loop

### State Store
9. **state_corruption** - Simulate corrupted state data
   - Justification: Test state validation and recovery
   - Location: state_store_get()

10. **state_persistence_failure** - Simulate file write failures
    - Justification: Test handling of read-only filesystems
    - Location: State serialization

### Network/API
11. **api_timeout** - Force immediate timeout
    - Justification: Test retry logic and timeout handling
    - Location: curl_easy_perform wrapper

12. **api_malformed_response** - Return invalid JSON
    - Justification: Test parser error recovery
    - Location: API response parsing

### Widget System
13. **widget_render_failure** - Simulate SDL render operation failure
    - Justification: Test render error propagation
    - Location: SDL render calls in widgets

14. **widget_memory_exhaustion** - Fail widget allocation
    - Justification: Test widget creation rollback
    - Location: widget_create()

### Configuration
15. **config_parse_error** - Inject syntax errors in config
    - Justification: Test config fallback mechanisms
    - Location: YAML parsing

16. **config_file_missing** - Simulate missing config files
    - Justification: Test default configuration usage
    - Location: Config file loading

### Font System
17. **font_load_failure** - Simulate font file corruption
    - Justification: Test fallback font handling
    - Location: TTF_OpenFont calls

18. **font_render_failure** - Simulate text rendering failure
    - Justification: Test text widget error handling
    - Location: TTF_RenderText calls

### General System
19. **random_allocation_failure** - Random malloc/calloc failures
    - Justification: Test memory allocation error paths
    - Location: Memory allocation wrappers

20. **thread_creation_failure** - Simulate pthread_create failures
    - Justification: Test async operation fallbacks
    - Location: Thread creation points

## Usage Example

```c
// Enable specific injection points via environment
export PANELKIT_INJECT_ERRORS="display_init_failure,api_timeout"

// Or via configuration
error_injection:
  enabled: true
  points:
    - display_init_failure
    - api_timeout
  probability: 0.1  # 10% chance when checked
```

## Benefits

1. **Deterministic Testing** - Reproduce exact error scenarios
2. **Coverage** - Test error paths that are hard to trigger naturally  
3. **Embedded Simulation** - Test embedded-specific failures on host
4. **Regression Testing** - Ensure error handling doesn't regress
5. **Stress Testing** - Combine multiple injection points

## Implementation Priority

High Priority (Critical Paths):
- display_init_failure
- input_device_disappear
- event_queue_overflow
- widget_memory_exhaustion

Medium Priority (Common Failures):
- api_timeout
- config_parse_error
- font_load_failure
- state_corruption

Low Priority (Edge Cases):
- thread_creation_failure
- random_allocation_failure
- touch_calibration_corrupt