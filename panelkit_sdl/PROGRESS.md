# State Management and Event System Implementation Progress

## Overview
This document captures the architectural decisions, implementation progress, and future roadmap for the state management and event system refactoring of PanelKit. It serves as a formal prompt to allow seamless continuation by another assistant.

## Current Status
- **Phase 1: State Store Foundation** - ✅ COMPLETE
- **Phase 2: Event System Core** - ✅ COMPLETE
- **Phase 3: Widget Interface Pattern** - 🚧 NEXT
- **Phase 4: Integration & Migration** - 📅 PLANNED

## Architecture Context

### Problem Statement
The original PanelKit implementation had tight coupling between:
- API calls and UI updates
- Button actions and data fetching
- State management scattered across components

This made it difficult to:
- Add new API services
- Update UI elements independently
- Handle asynchronous data flow cleanly
- Test components in isolation

### Solution Architecture
Implement a decoupled event-driven architecture with centralized state management:

```
[UI Components] <--subscribe--> [Event System] <--listen--> [State Store]
                                      ^
                                      |
                                 [API Parsers]
```

## Key Design Decisions

### 1. State Store Design (Phase 1 - COMPLETE)

**Compound Key Approach**
- Decision: Use "type_name:id" compound keys instead of nested structures
- Rationale: Simpler implementation, familiar from database/JS world, enables wildcards
- Example: `"weather_current:91007"`, `"battery_state:low"`

**Memory Management**
- Decision: Deep copy with thread safety (pthread_rwlock)
- Rationale: Devices have ample memory (several GB), safety over optimization
- Trade-off: More memory usage but simpler ownership model

**Type Registration**
- Decision: Runtime registration via string names (not fixed enum)
- Rationale: Flexibility for new APIs without recompilation
- Implementation: Dynamic type configuration with per-type settings

**Data Storage**
- Decision: Generic void* storage with type envelope
- Rationale: Most flexible for different data types
- Alternative considered: JSON strings (rejected for performance)

### 2. Event System Design (Phase 2 - NEXT)

**Event Naming**
- Decision: Hierarchical semantic names without tight coupling to source
- Examples: `"weather.temperature"`, `"device.battery"` (NOT `"api_weather_temperature"`)
- Rationale: Avoid coupling events to their source

**Subscription Model**
- Decision: Function pointer callbacks with context
- Pattern: `event_subscribe("weather.temperature", handler_func, context)`
- No wildcards initially (explicit subscriptions only)

**Publishing**
- Decision: Explicit publishing (not automatic on state change)
- Rationale: More control over event flow
- State store subscribes to all events for caching

### 3. Widget Interface Pattern (Phase 3 - PLANNED)

**Component Structure**
- Each UI element can subscribe to events
- Standard interface for event handling
- Components manage their own subscriptions

**Migration Strategy**
- Start with one dummy button as proof of concept
- Gradually migrate existing components
- Keep existing functionality working throughout

## Implementation Progress

### Phase 1: State Store Foundation ✅

**Completed Files:**
- `src/state/state_store.h` - Public API
- `src/state/state_store.c` - Implementation

**Key Features Implemented:**
```c
// Core operations with compound keys
state_store_set(store, "weather_current", "91007", data, size);
state_store_get(store, "weather_current", "91007", &size, &timestamp);

// Type configuration
DataTypeConfig config = {
    .type_name = "weather_current",
    .max_items_per_key = 1,      // Keep only latest
    .retention_seconds = 1800,   // 30 min expiration
    .cache_enabled = true
};

// Thread-safe with rwlocks
// Deep copy memory management
// Expiration handling
// Wildcard-ready structure
```

### Phase 2: Event System Core ✅

**Completed Files:**
- `src/events/event_system.h` - Public API
- `src/events/event_system.c` - Implementation
- `src/state/state_event_bridge.h` - Bridge between state and events
- `src/state/state_event_bridge.c` - Bridge implementation

**Implemented API:**
```c
// Event system lifecycle
EventSystem* event_system_create(void);
void event_system_destroy(EventSystem* system);

// Publishing
bool event_publish(EventSystem* system, const char* event_name, 
                   void* data, size_t data_size);

// Subscription
typedef void (*event_handler_func)(const char* event_name, 
                                  const void* data, size_t data_size,
                                  void* context);

bool event_subscribe(EventSystem* system, const char* event_name,
                    event_handler_func handler, void* context);
bool event_unsubscribe(EventSystem* system, const char* event_name,
                      event_handler_func handler);
```

**Key Features Implemented:**
- Thread-safe publish/subscribe with function pointer callbacks
- Multiple handlers per event, multiple contexts
- Deep copy event data for safety
- Statistics and debugging functions
- State-event bridge for automatic caching

**Integration with State Store:**
- Bridge allows state store to subscribe to events
- Automatic event-to-state mapping (event name → type_name:id)
- Configurable caching behavior
- Wildcard iteration support: `state_store_iterate_wildcard(store, "weather_*:*", ...)`

### Phase 3: Widget Interface Pattern 📅

**Planned Approach:**
```c
// Widget event interface
typedef struct {
    void (*on_event)(void* widget, const char* event_name, 
                     const void* data, size_t data_size);
    bool (*subscribe_events)(void* widget, EventSystem* system);
    void (*unsubscribe_events)(void* widget, EventSystem* system);
} WidgetEventInterface;

// Example button implementation
typedef struct {
    Button base;
    WidgetEventInterface events;
    char* subscribed_event;
} EventButton;
```

### Phase 4: Integration & Migration 📅

**API Integration Points:**
- API parsers publish events after successful parse
- Event names based on service/endpoint: `"weather.current_weather"`
- Parsers use meta/headers from config for custom behavior

**Example Flow:**
1. Button triggers API call: `api_request("weather", "current_weather", "91007")`
2. API response parsed by registered parser
3. Parser publishes: `event_publish("weather.temperature", &temp_data, sizeof(temp_data))`
4. State store caches: `state_store_set("weather_temperature", "91007", &temp_data, sizeof(temp_data))`
5. UI widgets update via their event handlers

## Configuration Integration

The new API configuration structure (already implemented) supports:
- Multiple services with multiple endpoints
- Custom headers and meta sections per service
- Parser registration per service/endpoint

Example:
```yaml
api:
  services:
    - id: "weather"
      host: "api.openweathermap.org"
      headers: '{"Accept": "application/json"}'
      meta: '{"rate_limit": 60, "default_city": "San Francisco,US"}'
      endpoints:
        - id: "current_weather"
          path: "/weather"
          method: "GET"
```

## Next Steps for Continuation

1. **Implement Phase 2: Event System Core**
   - Create event_system.h/c files
   - Implement publish/subscribe with function pointers
   - Add thread safety
   - Create unit tests

2. **Connect State Store to Event System**
   - State store subscribes to all events
   - Implement filtering for cache_enabled types
   - Add event history queries

3. **Create Proof of Concept**
   - One dummy button that subscribes to weather events
   - Test full flow: button → API → parser → event → state → UI update

4. **Plan Migration Strategy**
   - Identify order of component migration
   - Create compatibility layer for gradual transition
   - Document new patterns for team

## Technical Constraints & Decisions

**Language:** C (not C++)
- Use function pointers for polymorphism
- Explicit memory management
- Thread safety with pthreads

**Memory:** Ample available (several GB)
- Prefer safety over optimization
- Deep copies acceptable
- Can cache reasonable history

**Platform:** Embedded Linux (Yocto-based)
- SDL for display
- Touch input via evdev/SDL
- Network via curl

## Questions Resolved

1. **Compound keys vs nested structure** → Compound keys chosen
2. **Thread safety timing** → Implement now (not premature)
3. **Fixed types vs runtime registration** → Runtime registration
4. **Event wildcards** → Explicit subscriptions only (no wildcards initially)
5. **State ownership** → Deep copy (store owns its data)

## Architecture Benefits

1. **Decoupling:** UI doesn't know about APIs, APIs don't know about UI
2. **Testability:** Each component can be tested with mock events
3. **Extensibility:** New APIs/widgets added without touching existing code
4. **Performance:** Async updates, caching, selective refreshing
5. **Maintainability:** Clear separation of concerns

This architecture sets up PanelKit for long-term maintainability and feature additions while keeping the codebase manageable and testable.