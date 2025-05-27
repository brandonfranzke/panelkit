# Event System

## Overview

The event system provides a thread-safe publish/subscribe mechanism for decoupled communication between components. It supports both generic and type-safe event publishing with synchronous delivery.

## Architecture

```
Publishers                    Event System                  Subscribers
    │                             │                             │
    ├─ UI Events ─────────────────┤                             │
    ├─ State Changes ─────────────┤                             │
    ├─ API Updates ───────────────┼─────── Routing ────────────┤
    ├─ System Events ─────────────┤                             │
    └─ Widget Events ─────────────┤                             │
                                  │                             │
                           Topic Registry                 Handler Registry
                           - Event names                  - Callbacks
                           - Subscriber lists              - User contexts
```

## Core Concepts

### Event Names
Events use hierarchical dot-notation:
- `ui.button_pressed`
- `api.user_data_updated`
- `system.page_transition`
- `state.value_changed`

### Event Handler
```c
typedef void (*event_handler_func)(const char* event_name, 
                                  const void* data, 
                                  size_t data_size,
                                  void* context);
```

### Thread Safety
- Mutex protection for all operations
- Safe concurrent publish/subscribe
- No deadlock potential in design

## Usage

### Basic Publishing
```c
// Publish event without data
event_emit(event_system, "app.started");

// Publish with data
UserData user = { .name = "John", .age = 30 };
event_publish_with_data(event_system, "user.updated", 
                       &user, sizeof(user));
```

### Type-Safe Publishing
```c
// Button press event
ButtonEventData data = {
    .button_index = 0,
    .page = 1,
    .timestamp = time(NULL)
};
event_publish_button_pressed(event_system, &data);

// Page transition
PageTransitionData transition = {
    .from_page = 0,
    .to_page = 1
};
event_publish_page_transition(event_system, &transition);
```

### Subscribing
```c
// Subscribe to event
bool success = event_subscribe(event_system, "ui.button_pressed",
                              on_button_pressed, user_context);

// Handler implementation
void on_button_pressed(const char* event_name, 
                      const void* data, 
                      size_t data_size,
                      void* context) {
    ButtonEventData* event = (ButtonEventData*)data;
    printf("Button %d pressed on page %d\n", 
           event->button_index, event->page);
}
```

### Unsubscribing
```c
// Remove specific handler
event_unsubscribe(event_system, "ui.button_pressed", 
                  on_button_pressed, user_context);

// Remove all handlers for a context
event_unsubscribe_all(event_system, user_context);
```

## Event Types

### UI Events
```c
// Button press
typedef struct {
    int button_index;
    int page;
    time_t timestamp;
    char button_text[MAX_BUTTON_TEXT_LEN];
} ButtonEventData;

// Page transition  
typedef struct {
    int from_page;
    int to_page;
} PageTransitionData;

// Touch event
typedef struct {
    int x, y;
    int pressure;
    TouchState state;
} TouchEventData;
```

### API Events
```c
// User data update
typedef struct {
    char name[API_USER_NAME_MAX_LEN];
    int age;
    char email[API_USER_EMAIL_MAX_LEN];
    char phone[API_USER_PHONE_MAX_LEN];
    char location[API_USER_LOCATION_MAX_LEN];
    char nationality[API_USER_NATIONALITY_MAX_LEN];
    char picture_url[API_USER_PICTURE_URL_MAX_LEN];
} ApiUserDataUpdatedData;

// API state change
typedef struct {
    char state_name[API_STATE_NAME_MAX_LEN];
    char value[API_STATE_VALUE_MAX_LEN];
} ApiStateChangeData;
```

### System Events
- `system.startup` - Application initialized
- `system.shutdown` - Application terminating
- `system.page_transition` - Page change occurred
- `system.api_refresh` - API refresh requested

## Event Flow

1. **Publisher** calls event_publish()
2. **Event System** locks mutex
3. **Lookup** finds all subscribers for event
4. **Delivery** calls each handler synchronously
5. **Cleanup** unlocks mutex

## Best Practices

### DO:
- Use hierarchical event names
- Keep event data structures small
- Handle events quickly (no blocking operations)
- Copy data if needed beyond handler
- Unsubscribe in cleanup/destroy functions

### DON'T:
- Store pointers to event data
- Perform long operations in handlers
- Publish events while handling events
- Assume delivery order
- Modify event data in handlers

## Integration Examples

### Widget Event Publishing
```c
// In button widget click handler
if (button->base.event_system && button->publish_event_name) {
    ButtonEventData event_data = {
        .button_index = button->index,
        .page = button->page,
        .timestamp = time(NULL)
    };
    strncpy(event_data.button_text, button->text, 
            MAX_BUTTON_TEXT_LEN - 1);
    
    event_publish_with_data(button->base.event_system,
                           button->publish_event_name,
                           &event_data, sizeof(event_data));
}
```

### State Store Bridge
```c
// Automatic event-to-state bridging
event_subscribe(events, "api.user_data_updated", 
                bridge_event_handler, bridge_context);

// Bridge handler stores event data in state store
void bridge_event_handler(const char* event_name, 
                         const void* data, 
                         size_t data_size,
                         void* context) {
    BridgeContext* bridge = (BridgeContext*)context;
    state_store_set(bridge->store, "api_data", "user", 
                    data, data_size);
}
```

### Cross-Component Communication
```c
// API manager publishes data
event_publish_api_user_data_updated(event_system, &user_data);

// Multiple widgets subscribe
event_subscribe(events, "api.user_data_updated", 
                update_user_display, display_widget);
event_subscribe(events, "api.user_data_updated", 
                log_user_change, logger);
event_subscribe(events, "api.user_data_updated", 
                sync_to_state_store, state_bridge);
```

## Performance Considerations

- Events are delivered synchronously (no queue)
- Handlers run on the publishing thread
- Mutex held during entire delivery cycle
- No event prioritization or filtering
- Linear search for topic lookup

For high-frequency events, consider:
- Batching updates
- Using state store for polling
- Direct function calls for tight coupling
- Rate limiting event publishing