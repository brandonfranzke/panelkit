# Widget System

## Overview

The widget system provides a hierarchical, event-driven UI framework. All UI elements inherit from a base Widget type, enabling consistent behavior and rendering across the application.

## Widget Hierarchy

```
Widget (base)
├── Container
│   └── PageWidget
├── ButtonWidget
├── TextWidget (Label)
├── TimeWidget
├── DataDisplayWidget
├── WeatherWidget
└── PageManagerWidget
```

## Base Widget Structure

Every widget contains:

```c
struct Widget {
    // Identity
    char id[64];                    // Unique identifier
    WidgetType type;                // Widget type enum
    
    // Hierarchy
    Widget* parent;                 // Parent widget (null for root)
    Widget** children;              // Dynamic array of children
    size_t child_count;            // Number of children
    size_t child_capacity;         // Allocated capacity
    
    // Layout
    SDL_Rect bounds;               // Absolute screen position
    SDL_Rect relative_bounds;      // Position relative to parent
    int z_index;                   // Drawing order
    int padding;                   // Internal padding
    
    // State
    uint32_t state_flags;          // Combination of WidgetState flags
    bool visible;                  // Visibility flag
    bool enabled;                  // Interaction enabled
    
    // Rendering
    SDL_Color background_color;    // Background color
    bool has_background;           // Whether to draw background
    
    // Function pointers
    widget_render_func render;     // Custom render function
    widget_update_func update;     // Time-based update
    widget_destroy_func destroy;   // Custom cleanup
    
    // Event handling
    widget_event_handler on_event; // SDL event handler
    EventSystem* event_system;     // For publishing events
    StateStore* state_store;       // For state access
};
```

## Widget Lifecycle

### Creation
1. Allocate widget structure
2. Initialize base properties
3. Set type-specific defaults
4. Register with parent (if any)
5. Connect to systems (events, state)

### Rendering
1. Check visibility flag
2. Apply clipping to parent bounds
3. Render background (if enabled)
4. Call custom render function
5. Render children in z-order
6. Restore clipping region

### Event Handling
1. Widget manager routes SDL events
2. Hit testing determines target widget
3. State flags updated (hover, pressed)
4. Custom event handler called
5. Events may bubble to parent

### Destruction
1. Remove from parent
2. Destroy all children
3. Call custom destroy function
4. Free allocated resources

## Widget Types

### Container
Basic container for grouping widgets. No visual representation beyond optional background.

**Use cases**: Layout grouping, clipping regions, pages

### ButtonWidget
Interactive button with text label and customizable appearance.

**Properties**:
- Normal, hover, and pressed colors
- Click callback with user data
- Optional event publishing
- Customizable padding

**Events**: Can publish custom events on click

### TextWidget (Label)
Simple text display with alignment options.

**Properties**:
- Text content
- Font selection
- Color
- Alignment (left, center, right)

**Features**: Automatic text texture caching

### TimeWidget
Real-time clock display with format string.

**Properties**:
- strftime format string
- Update frequency
- Font selection

**Example formats**:
- `"%H:%M:%S"` - 24-hour time
- `"%I:%M %p"` - 12-hour with AM/PM
- `"%Y-%m-%d"` - Date

### PageManagerWidget
Manages multiple pages with swipe transitions.

**Features**:
- Horizontal swipe gestures
- Animated transitions
- Page indicators
- Current page tracking
- Page change callbacks

**Gesture handling**:
- Swipe threshold: 30% of screen width
- Elastic resistance at boundaries
- Smooth transition animations

### DataDisplayWidget
Formatted display of user data from API.

**Layout**:
- Label-value pairs
- Consistent spacing
- Automatic null handling

## Event Integration

Widgets can interact with the event system in two ways:

### 1. Publishing Events
```c
// In button click handler
if (button->base.event_system && button->publish_event_name) {
    event_publish_with_data(button->base.event_system,
                           button->publish_event_name,
                           button->publish_data,
                           button->publish_data_size);
}
```

### 2. Subscribing to Events
```c
// During widget initialization
event_subscribe(event_system, "data.updated", 
                on_data_updated, widget);
```

## State Store Integration

Widgets can read from and write to the state store:

```c
// Reading state
size_t size;
time_t timestamp;
bool* value = state_store_get(store, "app", "show_time", 
                              &size, &timestamp);

// Writing state
bool new_value = true;
state_store_set(store, "app", "show_time", 
                &new_value, sizeof(bool));
```

## Custom Widget Creation

To create a custom widget:

1. Define widget structure extending Widget:
```c
typedef struct {
    Widget base;  // Must be first
    // Custom fields
} CustomWidget;
```

2. Implement creation function:
```c
Widget* custom_widget_create(const char* id) {
    CustomWidget* widget = calloc(1, sizeof(CustomWidget));
    widget_init(&widget->base, id, WIDGET_TYPE_CUSTOM);
    
    // Set custom functions
    widget->base.render = custom_render;
    widget->base.destroy = custom_destroy;
    widget->base.on_event = custom_event_handler;
    
    return (Widget*)widget;
}
```

3. Implement required functions:
- `render`: Draw the widget
- `destroy`: Clean up resources
- `on_event`: Handle SDL events (optional)
- `update`: Time-based updates (optional)

## Rendering Pipeline

1. **Setup**: Save current clip region
2. **Clipping**: Intersect with widget bounds
3. **Background**: Draw if enabled
4. **Content**: Call widget's render function
5. **Children**: Render in z-order
6. **Cleanup**: Restore clip region

## Best Practices

1. **ID Naming**: Use hierarchical IDs (e.g., "main.header.title")
2. **Memory**: Parent widgets own their children
3. **Events**: Cleanup subscriptions in destroy function
4. **State**: Use state store for shared data
5. **Rendering**: Minimize texture creation/destruction
6. **Updates**: Only mark dirty when state changes