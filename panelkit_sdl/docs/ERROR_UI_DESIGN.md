# Error UI Design and Architecture

## Overview

This document outlines the design and implementation approach for user-visible error handling in PanelKit. The error UI system is designed to provide clear, non-intrusive feedback to users while maintaining system responsiveness and debuggability.

## Error Notification Widget System

### Design Goals
- Non-blocking display of errors
- Automatic categorization by severity
- Queue management for multiple simultaneous errors
- Minimal performance impact
- Touch-friendly interaction

### Architecture

#### Error Queue Manager
```c
typedef struct {
    PkError code;
    char context[256];
    time_t timestamp;
    ErrorSeverity severity;
    bool acknowledged;
} ErrorQueueEntry;

typedef struct {
    ErrorQueueEntry entries[MAX_ERROR_QUEUE];
    size_t head;
    size_t tail;
    size_t count;
} ErrorQueue;
```

#### Widget Integration
The error widget should be a specialized widget type that:
- Monitors the global error queue
- Renders appropriate UI based on severity
- Handles touch input for dismissal/expansion
- Integrates with the widget tree as a high-z-order overlay

### Display Strategies

#### Transient Notifications (Toast)
- Auto-dismiss after 3-5 seconds
- Slide in from top/bottom edge
- Stack vertically if multiple errors
- Severity indicated by color/icon

#### Persistent Banners
- Require explicit dismissal
- Used for critical errors requiring action
- Positioned to not obscure primary UI
- Include actionable buttons (Retry, Dismiss, Details)

#### Error Detail Expansion
- Tap to expand for full error context
- Show timestamp, error code, detailed message
- Provide copy-to-clipboard functionality
- Link to relevant documentation/help

### Hook Points

```c
// Global error notification callback
typedef void (*error_notification_callback)(PkError error, const char* context);

// Register callback for UI notifications
void pk_error_set_notification_callback(error_notification_callback callback);

// Queue an error for display
void pk_error_queue_for_display(PkError error, const char* context, ErrorSeverity severity);

// Check if errors are pending display
bool pk_error_has_pending_notifications(void);
```

## Status Bar Integration

### Design Principles
- Always visible but unobtrusive
- Ambient indication of system health
- Progressive disclosure of details
- Consistent positioning across pages

### Information Architecture

#### Primary Indicators
1. **Connection Status**: Icon showing API connectivity
2. **Error Badge**: Count of unacknowledged errors
3. **System Status**: Overall health indicator

#### Expandable Details
- Tap status bar to reveal:
  - Recent error summary
  - Network statistics
  - Resource usage
  - Quick actions (clear errors, retry connection)

### Implementation Hooks

```c
typedef struct {
    bool api_connected;
    int error_count;
    int warning_count;
    SystemHealth health;
    time_t last_error_time;
} StatusBarState;

// Update status bar state
void pk_status_bar_update(const StatusBarState* state);

// Register status bar widget
Widget* pk_status_bar_create(const char* id);
```

## Integration with Error System

### Automatic Notification
Enhance `pk_set_last_error_with_context` to optionally trigger UI notifications:

```c
// Extended error setting with UI notification
void pk_set_last_error_with_notification(PkError error, 
                                       ErrorSeverity severity,
                                       const char* fmt, ...);
```

### Severity Classification

```c
typedef enum {
    ERROR_SEVERITY_INFO,     // Informational only
    ERROR_SEVERITY_WARNING,  // Degraded functionality
    ERROR_SEVERITY_ERROR,    // Feature unavailable
    ERROR_SEVERITY_CRITICAL  // System-wide impact
} ErrorSeverity;

// Determine severity from error code
ErrorSeverity pk_error_get_severity(PkError error);
```

### Context Enhancement
Errors should provide user-friendly messages in addition to technical context:

```c
typedef struct {
    const char* technical_message;  // For logs/developers
    const char* user_message;       // For UI display
    const char* recovery_hint;      // What user can do
} ErrorContext;
```

## Touch Interaction Design

### Gesture Support
- **Tap**: Expand/collapse error details
- **Swipe**: Dismiss transient notifications
- **Long press**: Copy error details
- **Pinch**: Zoom error detail text

### Accessibility
- Minimum touch target: 44x44 points
- High contrast colors
- Clear iconography
- Optional haptic feedback

## Performance Considerations

### Resource Limits
- Maximum 10 queued errors
- Auto-cleanup of acknowledged errors after 1 minute
- Lazy rendering of off-screen notifications
- Texture caching for common error types

### Memory Management
- Pre-allocate error queue at startup
- Reuse notification widget instances
- Clear old errors on memory pressure

## Future Enhancements

### Localization
- Error message string tables
- RTL language support
- Cultural icon adaptation

### Analytics
- Error frequency tracking
- User interaction patterns
- Recovery success rates

### Remote Reporting
- Optional error telemetry
- Crash report generation
- Field debugging support

## Implementation Phases

1. **Phase 1**: Hook infrastructure and stubs
2. **Phase 2**: Basic error widget with manual display
3. **Phase 3**: Automatic error detection and display
4. **Phase 4**: Full interaction and animation support

This design provides a foundation for comprehensive error UI while remaining flexible enough to adapt to evolving layout requirements.