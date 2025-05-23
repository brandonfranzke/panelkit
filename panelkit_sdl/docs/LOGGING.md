# PanelKit Logging System

## Overview

PanelKit uses [zlog](https://github.com/HardySimpson/zlog) as its logging backend, providing:
- Thread-safe logging across multiple components
- Automatic log rotation with size limits
- Multiple output destinations (files, stderr)
- Configurable formatting and filtering
- High performance with minimal overhead

## Architecture

```
Application Code
    ↓
logger.h API (Abstraction Layer)
    ↓
zlog Library (Thread-Safe Backend)
    ↓
Multiple Outputs (Files, Console)
```

## Installation

### Development System (macOS/Linux)
```bash
./scripts/install_zlog.sh
```

### Target System (Raspberry Pi)
```bash
# Include in deployment
scp scripts/install_zlog.sh brandon@panelkit:/tmp/
ssh brandon@panelkit "cd /tmp && ./install_zlog.sh"
```

## Configuration

### Configuration File Location
- Development: `config/zlog.conf`
- Deployed: `/etc/panelkit/zlog.conf`
- Fallback: Internal default configuration

### Log File Locations
- Main log: `/var/log/panelkit/panelkit.log`
- Error log: `/var/log/panelkit/error.log`
- Fatal log: `/var/log/panelkit/fatal.log`
- Performance: `/var/log/panelkit/performance.log` (optional)

### Log Rotation
- Main log: 10MB × 5 files
- Error log: 5MB × 3 files
- Fatal log: No rotation (preserved for debugging)

## API Usage

### Basic Logging
```c
#include "core/logger.h"

// Initialize at startup
if (!logger_init("config/zlog.conf", "panelkit")) {
    // Falls back to stderr logging
}

// Log messages
log_debug("Detailed debugging information");
log_info("Normal operational messages");
log_notice("Significant events");
log_warn("Warning conditions");
log_error("Error conditions");
log_fatal("Fatal errors (aborts program)");

// Shutdown before exit
logger_shutdown();
```

### System Information
```c
// Log system details at startup
log_system_info();   // OS, hostname, PID
log_build_info();    // Version, build date
log_display_info(width, height, "SDL+DRM");
```

### Error Helpers
```c
// SDL errors
if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    LOG_SDL_ERROR("Failed to initialize SDL");
}

// System errors with errno
if (open(filename, O_RDONLY) < 0) {
    LOG_ERRNO("Failed to open file");
}

// DRM errors
if (drmModeSetCrtc(...) != 0) {
    LOG_DRM_ERROR("Failed to set CRTC", errno);
}
```

### Assertions
```c
// Assert with logging
LOG_ASSERT(ptr != NULL, "Pointer must not be NULL");
LOG_ASSERT(value >= 0, "Value must be non-negative: %d", value);
```

### State & Event Tracking
```c
// Log state changes
log_state_change("Display", "INITIALIZING", "READY");

// Log events with context
log_event("BUTTON_PRESS", "id=%d x=%d y=%d", btn_id, x, y);
```

### Performance Monitoring
```c
// Frame time tracking (auto-aggregates)
Uint32 frame_start = SDL_GetTicks();
// ... render frame ...
log_frame_time(SDL_GetTicks() - frame_start);

// Memory usage
log_memory_usage();  // Logs VmSize and VmRSS
```

## Integration Example

```c
int main(int argc, char* argv[]) {
    // Initialize logging first
    if (!logger_init(NULL, "panelkit")) {
        fprintf(stderr, "Warning: Using fallback logging\n");
    }
    
    // Log startup
    log_info("=== PanelKit Starting ===");
    log_system_info();
    log_build_info();
    
    // Initialize subsystems with logging
    log_state_change("SDL", "NONE", "INITIALIZING");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_SDL_ERROR("SDL initialization failed");
        logger_shutdown();
        return 1;
    }
    log_state_change("SDL", "INITIALIZING", "READY");
    
    // Main loop with performance tracking
    while (running) {
        Uint32 frame_start = SDL_GetTicks();
        
        // ... application logic ...
        
        log_frame_time(SDL_GetTicks() - frame_start);
    }
    
    // Clean shutdown
    log_info("=== PanelKit Shutting Down ===");
    logger_shutdown();
    return 0;
}
```

## Log Analysis

### View logs in real-time
```bash
# On target
tail -f /var/log/panelkit/panelkit.log

# Filter by level
grep " ERROR " /var/log/panelkit/panelkit.log
grep " WARN " /var/log/panelkit/panelkit.log

# View with less
less /var/log/panelkit/panelkit.log
```

### Transfer logs for analysis
```bash
# From development machine
scp brandon@panelkit:/var/log/panelkit/*.log ./logs/
```

### Parse structured events
```bash
# Find all state changes
grep "\[STATE:" /var/log/panelkit/panelkit.log

# Find all events
grep "\[EVENT:" /var/log/panelkit/panelkit.log
```

## Best Practices

1. **Initialize Early**: Start logging before any other subsystem
2. **Log Levels**: Use appropriate levels (debug for development, info for production)
3. **Context**: Include relevant context in messages (IDs, coordinates, states)
4. **Errors**: Always log errors with enough context to debug remotely
5. **Performance**: Use log_frame_time() sparingly in production
6. **Cleanup**: Always call logger_shutdown() on exit

## Troubleshooting

### No log output
1. Check log directory exists: `mkdir -p /var/log/panelkit`
2. Check permissions: `ls -la /var/log/panelkit`
3. Verify zlog installation: `ldconfig -p | grep zlog`

### Log rotation not working
1. Check lock file: `ls -la /tmp/panelkit_zlog.lock`
2. Remove stale lock: `rm /tmp/panelkit_zlog.lock`

### Performance impact
1. Reduce log level in production
2. Disable frame time logging
3. Increase buffer sizes in zlog.conf