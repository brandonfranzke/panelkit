# Widget System Technical Debt

## Active Issues

### Architecture & Design

1. **No Layout Management**
   - **Issue**: All positioning is manual with hardcoded values
   - **Impact**: No responsive design, lots of magic numbers
   - **Solution**: Implement proper layout managers (flex, grid, etc.)

2. **Event System String Coupling**
   - **Issue**: Events use string names, prone to typos
   - **Impact**: Runtime errors, no compile-time checking
   - **Solution**: Use enums or strongly-typed events

3. **No Style/Theme System**
   - **Issue**: Colors and sizes hardcoded everywhere
   - **Impact**: Can't change appearance without recompiling
   - **Solution**: Implement proper theming/styling system

### Code Quality

4. **Duplicate Logging Macros**
   - **Issue**: Every file redefines logging macros
   - **Impact**: Code duplication, inconsistent logging
   - **Solution**: Use centralized logger.h properly

5. **Memory Management**
   - **Issue**: Manual malloc/free without consistent patterns
   - **Impact**: Potential leaks, crashes
   - **Solution**: Implement consistent allocation/cleanup patterns

6. **No Error Recovery**
   - **Issue**: Many functions silently fail or just return NULL
   - **Impact**: Hard to debug, poor user experience
   - **Solution**: Proper error handling and reporting

7. **Type Safety**
   - **Issue**: Excessive use of void* and manual casting
   - **Impact**: Runtime errors, hard to refactor
   - **Solution**: Use proper types and interfaces

### Performance

8. **No Dirty Region Tracking**
    - **Issue**: Always redraw entire screen
    - **Impact**: Poor performance, battery drain
    - **Solution**: Implement dirty rectangle system

9. **Inefficient Hit Testing**
    - **Issue**: Linear search through widget tree
    - **Impact**: Slow with many widgets
    - **Solution**: Spatial indexing or caching

### Testing & Documentation

10. **No Unit Tests**
    - **Issue**: No automated testing for widgets
    - **Impact**: Regressions, hard to refactor
    - **Solution**: Add unit test framework and tests

11. **Inconsistent API Documentation**
    - **Issue**: Some functions documented, many aren't
    - **Impact**: Have to read source to understand API
    - **Solution**: Document all public APIs consistently

## Next Planned Improvements

### 1. Type Safety (Phase 1+2)
**Approach**: Fix the most dangerous void* usage patterns in phases

**Phase 1 - Easy Wins**:
- Replace all anonymous structs with named types
  ```c
  // Current: dangerous anonymous struct
  struct { int button_index; int page; } *data = (void*)ptr;
  
  // Target: named type
  typedef struct {
      int button_index;
      int page;
      uint32_t timestamp;
      char button_text[32];
  } ButtonEventData;
  ```
- Add type-safe casting macros for all widget types
  ```c
  #define CAST_BUTTON(widget) \
      ((widget) && (widget)->type == WIDGET_TYPE_BUTTON ? (ButtonWidget*)(widget) : NULL)
  ```
- Document all void* usage with ownership and type comments

**Phase 2 - Critical Safety**:
- Fix unsafe casts in event publishing (button_widget.c:156)
- Add runtime type checking to widget casts
- Create typed factory functions alongside generic ones
- Replace void* impl_data with typed alternatives where possible

**Files to modify**:
- All widget implementations (button_widget.c, text_widget.c, etc.)
- widget_factory.h/c for typed creation functions
- widget.h for casting macros

### 2. Event System String Coupling
**Approach**: Hybrid enum + string system for compile-time safety with debugging

**Implementation**:
- Create event type enum in events/event_types.h
  ```c
  typedef enum {
      // UI Events
      EVENT_UI_BUTTON_PRESSED,
      EVENT_UI_PAGE_CHANGED,
      
      // App Events  
      EVENT_APP_PAGE_TRANSITION,
      
      // API Events
      EVENT_API_REFRESH_REQUESTED,
      EVENT_API_STATE_CHANGED,
      EVENT_API_USER_DATA_UPDATED,
      
      EVENT_TYPE_COUNT
  } EventType;
  
  // Keep string names for debugging
  extern const char* event_type_names[EVENT_TYPE_COUNT];
  ```
- Add new type-safe publish/subscribe functions
  ```c
  bool event_publish_typed(EventSystem* system, EventType type,
                          const void* data, size_t data_size);
  bool event_subscribe_typed(EventSystem* system, EventType type,
                            event_handler_func handler, void* context);
  ```
- Keep string-based API for backward compatibility during migration
- Update all event publishers/subscribers to use enums

**Current event names to migrate**:
- ui.button_pressed → EVENT_UI_BUTTON_PRESSED
- ui.page_changed → EVENT_UI_PAGE_CHANGED
- app.page_transition → EVENT_APP_PAGE_TRANSITION
- api.refresh_requested → EVENT_API_REFRESH_REQUESTED
- api.state_changed → EVENT_API_STATE_CHANGED
- api.user_data_updated → EVENT_API_USER_DATA_UPDATED
- system.api_refresh → EVENT_SYSTEM_API_REFRESH
- weather.request → EVENT_WEATHER_REQUEST

### 3. Memory Management - Pool Allocator
**Approach**: Arena allocator for widgets with clear lifetime boundaries

**Design**:
```c
typedef struct {
    uint8_t* memory;
    size_t capacity;
    size_t used;
    size_t alignment;
} MemoryPool;

// Pool management
MemoryPool* pool_create(size_t size);
void* pool_alloc(MemoryPool* pool, size_t size);
void pool_reset(MemoryPool* pool);  // Clear all allocations
void pool_destroy(MemoryPool* pool);
```

**Implementation strategy**:
- Create separate pools for different lifetimes:
  - Widget pool: All widgets for a page (reset on page change)
  - Event pool: Temporary event data (reset after dispatch)
  - System pool: Long-lived system objects
- Add pool parameter to widget creation functions
- Replace individual widget free() calls with pool resets
- Keep traditional malloc/free for system objects with complex lifetimes

**Integration points**:
- widget_manager to own and manage widget pools
- widget_factory to accept pool parameter
- Page transitions to trigger pool resets
- Event system to use temporary event pool

**Benefits over current approach**:
- No individual free() calls needed for widgets
- Guaranteed cleanup on page transitions
- Better cache locality for related widgets
- Eliminates most leak potential in UI code

## Priority Order

### Completed ✅
- Logging cleanup (#4) - Fixed duplicate macros

### High Priority (Next Tasks)
1. Type Safety Phase 1+2 (#7) - Foundation for other improvements
2. Event System String Coupling (#2) - Builds on type definitions
3. Memory Management Pool Allocator (#5) - Uses types and events

### Medium Priority (Future)
4. Error handling (#6)
5. Layout management (#1)

### Low Priority (Nice to Have)
6. Theme system (#3)
7. Performance optimizations (#8, #9)
8. Testing infrastructure (#10)
9. API documentation (#11)

## Optional Enhancements

### UI Features
1. **Page Indicators**
   - Pills with dots at bottom of screen
   - Show during swipe and 2 seconds after
   - Fade out with 400ms duration

2. **Widget Debug Overlay**
   - Widget-based FPS counter
   - Current page indicator
   - Touch state display
