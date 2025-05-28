# PanelKit Design Patterns and Methodology Guide for AI Assistants

This document captures specific, opinionated design patterns and implementation requirements for PanelKit. It is written for AI assistants to understand the precise engineering standards required.

## Core Philosophy

**Priority**: Engineering excellence over quick implementation. The user explicitly values "comprehensive and systematic changes" over simpler approaches. Never take shortcuts.

## Critical Design Patterns

### 1. Error Handling (STRICT REQUIREMENTS)

**Pattern**: Two-step error reporting
```c
// ALWAYS use this pattern:
if (error_condition) {
    pk_set_last_error_with_context(ERROR_CODE, "Descriptive message with %s", context);
    return ERROR_CODE;  // pk_set_last_error_with_context returns void!
}
```

**NEVER**:
```c
// WRONG - function returns void
return pk_set_last_error_with_context(ERROR_CODE, "message");

// WRONG - no error context
return ERROR_CODE;

// WRONG - generic message
pk_set_last_error_with_context(ERROR_CODE, "Error");
```

**Error Codes**: Use existing PkError enum values:
- `PK_ERROR_NULL_PARAM` - for NULL parameter checks
- `PK_ERROR_INVALID_PARAM` - for invalid values
- `PK_ERROR_OUT_OF_MEMORY` - for allocation failures
- `PK_ERROR_INVALID_STATE` - for state violations

### 2. Memory Management

**Ownership Rules**:
- Every allocation must have clear ownership
- Parent owns children (Widget owns its children array)
- Specs/contexts own their type-specific data
- No "floating" allocations

**Pattern**: Explicit create/destroy pairs
```c
// ALWAYS provide matching pairs:
LayoutSpec* layout_spec_create(LayoutType type);
void layout_spec_destroy(LayoutSpec* spec);

// Destroy must handle NULL gracefully:
void layout_spec_destroy(LayoutSpec* spec) {
    if (!spec) return;
    
    // Type-specific cleanup
    switch (spec->type) {
        case LAYOUT_FLEX:
            free(spec->data.flex.child_props);
            break;
        // ...
    }
    
    free(spec);
}
```

**NEVER use realloc without checking**:
```c
// CORRECT pattern:
size_t new_capacity = old_capacity ? old_capacity * 2 : 16;
FlexChildProp* new_props = realloc(props, new_capacity * sizeof(FlexChildProp));
if (!new_props) {
    pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY, 
                                   "Failed to grow flex child properties");
    return PK_ERROR_OUT_OF_MEMORY;
}
props = new_props;
```

### 3. Type Safety

**Prefer unions over void***:
```c
// PREFERRED - type-safe union
typedef struct {
    LayoutType type;
    union {
        AbsoluteData absolute;
        FlexData flex;
    } data;
} LayoutSpec;

// AVOID - void* requires casting
typedef struct {
    LayoutType type;
    void* type_data;  // Avoid unless absolutely necessary
} LayoutSpec;
```

**Rationale**: User has "went through a substantial effort to minimize/eliminate void*"

### 4. Logging

**ALWAYS use PanelKit logging**:
```c
// CORRECT
PK_LOG_DEBUG("Layout calculation: width=%f, height=%f", width, height);
PK_LOG_ERROR("Failed to allocate layout context");

// NEVER
printf("Debug: %s\n", message);      // WRONG
fprintf(stderr, "Error: %s\n", msg);  // WRONG
#ifdef DEBUG
    printf(...);  // WRONG - use PK_LOG_DEBUG instead
#endif
```

**Log Levels**:
- `PK_LOG_DEBUG` - Detailed debugging info
- `PK_LOG_INFO` - Normal operational messages
- `PK_LOG_WARN` - Warning conditions
- `PK_LOG_ERROR` - Error conditions (use with error returns)

### 5. State Management

**NO GLOBAL VARIABLES**:
```c
// NEVER DO THIS:
static SomeData* g_global_data = NULL;
static int g_count = 0;

// CORRECT - pass through context:
typedef struct {
    SomeData* data;
    int count;
} Context;
```

**Rationale**: Global state makes testing difficult and violates PanelKit's design

### 6. Function Signatures

**Input validation pattern**:
```c
PkError some_function(Widget* widget, LayoutSpec* spec, Context* ctx) {
    // ALWAYS validate ALL pointer parameters first
    if (!widget || !spec || !ctx) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in some_function");
        return PK_ERROR_NULL_PARAM;
    }
    
    // Then validate state/values
    if (spec->type != expected_type) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid spec type %d, expected %d", 
                                       spec->type, expected_type);
        return PK_ERROR_INVALID_PARAM;
    }
    
    // Actual logic...
}
```

### 7. Testing Patterns

**Test independence**:
- Each test must be completely independent
- No shared state between tests
- Use setUp/tearDown for common initialization
- Clean up all allocations

**Test naming**:
```c
void test_module_specific_behavior(void) {
    // Arrange
    
    // Act
    
    // Assert
    
    // Cleanup
}
```

### 8. Configuration Philosophy

**Compile-time over runtime**:
- Style constants in header files, not config files
- No runtime theme loading - recompile to change
- Rationale: "I dont have a problem to having to recompile if I want to retheme"

**Property hierarchy** (when needed):
1. Widget-specific override
2. Parent specification  
3. Theme constants
4. Hardcoded defaults

### 9. Layout Specific Rules

**No circular dependencies**:
- Parents can NEVER reference child sizes
- Children CAN reference parent (e.g., 50% width)
- This prevents circular dependency issues entirely

**Overflow handling**:
- Default: clip_overflow = true (crop/truncate)
- Allow opt-out: clip_overflow = false (spillover)
- No complex constraint validation (110% width is allowed)

**Coordinate system**:
- Floats for internal calculations
- 0.0-1.0 for relative positioning
- >1.0 for absolute pixels
- Convert to SDL_Rect integers only at final step

### 10. Documentation

**Function documentation**:
```c
/**
 * Brief description of function purpose.
 * 
 * @param widget Widget to process (required)
 * @param spec Layout specification (required) 
 * @param results Output array, must have space for widget+children
 * @return PK_OK on success, error code on failure
 * @note Caller must ensure results array is large enough
 */
```

**Implementation comments**:
- Explain WHY, not WHAT
- No obvious comments like `// Increment counter`
- Document tricky algorithms or non-obvious decisions

## Common Mistakes to Avoid

1. **Using printf instead of PK_LOG_***
2. **Returning void function results as error codes**
3. **Global variables for state management**
4. **Not checking allocation failures**
5. **Generic error messages without context**
6. **void* when unions would work**
7. **Missing NULL checks on parameters**
8. **Inconsistent ownership patterns**
9. **No matching destroy for create functions**
10. **Test state leaking between test cases**

## Specific Implementation Patterns

### Creating a New Layout Type

1. Define type-specific data struct (no void*)
2. Add to LayoutType enum
3. Add to union in LayoutSpec
4. Implement create function with error checking
5. Implement destroy function handling NULL
6. Implement calculate function with full validation
7. Write comprehensive tests
8. Update documentation

### Adding Widget Properties

1. Store in parent's spec (not global)
2. Clear ownership model
3. Provide setter functions with validation
4. No property cascade complexity
5. Direct lookups, no string keys

## Testing Requirements

- Test every error path
- Test boundary conditions  
- Test with NULL parameters
- Test memory allocation failures (where possible)
- Use Unity TEST_ASSERT macros consistently
- Clean up all test allocations

## Final Notes

The user values "strong engineering and design principles" above getting things done quickly. When in doubt:
- Choose type safety over flexibility
- Choose explicit over implicit
- Choose simple rules over complex validation
- Choose clear ownership over shared state
- Choose comprehensive tests over quick implementation

Remember: "My highest priority is adherence to strong engineering and design principles. That supersedes efforts to just 'get it done'"