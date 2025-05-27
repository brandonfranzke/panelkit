# Widget System Documentation

## Current Status

The widget system is complete and functional. Widget mode (`WIDGET_RENDER=1`) runs independently from legacy code and is ready to replace the legacy rendering system.

## Working Features ✅

- **Button Functionality**: All buttons work correctly
  - "Change Color" - transitions to page 1
  - Color buttons - change background color
  - "Time" - toggles time display
  - "Go to Page 1" - returns to page 0
  - "Refresh User" - fetches new API data
  - "Exit App" - quits application
  
- **Navigation**: Swipe gestures for page transitions
- **Architecture**: Composable widgets (buttons contain text widgets)
- **State Management**: Full state store integration
- **Event System**: Event-driven architecture with proper propagation
- **Independence**: Complete separation from legacy code in widget mode

## Known Issues

1. **Swipe gestures occasionally "jumpy"** - needs tuning
2. **Page indicators not implemented** in widget system
3. **Debug overlay shows legacy state** - needs widget version

## Architecture

### Widget Composition
- Buttons are containers that hold text widgets
- Child widgets use relative positioning to parent
- `widget_update_child_bounds()` recursively updates positions when parents move

### Event Flow
```
1. SDL Event → widget_manager_handle_event
2. Find hit widget via widget_hit_test (prefers interactive widgets)
3. Set PRESSED state on hit widget
4. Call widget_handle_event (propagates to all widgets)
5. Button checks PRESSED state and fires click handler
6. Event system notifies subscribers
```

### Page Management
- PageManagerWidget handles all transitions
- Pages positioned horizontally (x = page_index * screen_width)
- Swipe gestures update transition_offset (normalized -1.0 to 1.0)
- Smooth animation between pages

### State Store Integration
In widget mode, the system:
- Reads ALL state from state store (never from globals)
- Updates only state store (never globals)
- Skips legacy page transition functions
- Doesn't publish legacy system events

## Testing

```bash
# Build
make clean && make host

# Run in widget mode
WIDGET_RENDER=1 ./build/host/panelkit

# Test all functionality:
1. Click "Change Color" button - verify transition to page 1
2. Click color buttons - verify background changes
3. Swipe left/right - verify smooth page transitions
4. Click "Go to Page 1" - verify return to page 0
5. Click "Exit App" - verify application quits
```

## Key Implementation Details

### Button Event Handling (button_widget.c)
```c
// Handle click BEFORE calling default handler (which clears PRESSED state)
if (event->type == SDL_MOUSEBUTTONUP && 
    event->button.button == SDL_BUTTON_LEFT) {
    bool has_pressed = widget_has_state(widget, WIDGET_STATE_PRESSED);
    bool contains_point = widget_contains_point(widget, event->button.x, event->button.y);
    if (has_pressed && contains_point) {
        button_widget_click(button);
    }
}
```

### Hit Testing (widget.c)
```c
// Check if this widget is truly interactive (not just a container with scroll handling)
bool is_interactive = (root->type == WIDGET_TYPE_BUTTON);

// If we're interactive, return this widget unless child is also interactive
if (is_interactive && root->handle_event) {
    if (!child_hit || child_hit->type != WIDGET_TYPE_BUTTON) {
        return root;
    }
}
```

### Widget Mode Independence (widget_integration.c)
```c
// Only mirror to legacy system if NOT in widget render mode
if (!getenv("WIDGET_RENDER")) {
    if (pages_get_current() != to_page) {
        pages_transition_to(to_page);
    }
    widget_integration_mirror_page_change(integration, from_page, to_page);
} else {
    // In widget mode, just update state store
    state_store_set(integration->state_store, "app", "current_page", &to_page, sizeof(int));
}
```

## Files

### Core Widget System
- `src/ui/widget.c` - Base widget functionality, hit testing, child bounds
- `src/ui/widget.h` - Widget interface definitions
- `src/ui/widget_manager.c` - Event routing and state management
- `src/ui/widget_factory.c` - Widget creation utilities
- `src/ui/widget_integration.c` - Integration layer (needs refactoring - see TECH_DEBT.md)

### Widget Types
- `src/ui/widgets/button_widget.c` - Interactive button containers
- `src/ui/widgets/text_widget.c` - Text rendering widgets
- `src/ui/widgets/page_manager_widget.c` - Page navigation and gestures
- `src/ui/widgets/data_display_widget.c` - API data display
- `src/ui/widgets/time_widget.c` - Time display widget
- `src/ui/widgets/weather_widget.c` - Weather data widget (example)

## Next Steps

See MIGRATION_PLAN.md for detailed legacy removal plan.