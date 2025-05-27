/**
 * @file memory_patterns.h
 * @brief Memory ownership and lifetime patterns
 * 
 * Documents memory management conventions used throughout the codebase.
 */

#ifndef MEMORY_PATTERNS_H
#define MEMORY_PATTERNS_H

/**
 * PanelKit Memory Management Patterns
 * 
 * This document defines the consistent memory ownership patterns used throughout
 * the PanelKit codebase. Every dynamic allocation should follow one of these patterns.
 */

/* ============================================================================
 * PATTERN 1: Parent Owns Child
 * ============================================================================
 * The parent object allocates and owns the child object.
 * The parent's destroy function is responsible for freeing the child.
 * 
 * Rules:
 * - Parent allocates child using malloc/calloc
 * - Parent stores pointer to child
 * - Parent's destroy function calls child's destroy function
 * - Child NEVER frees itself
 * 
 * Example:
 *   Widget* parent = widget_create();
 *   Widget* child = button_widget_create();
 *   widget_add_child(parent, child);  // parent now owns child
 *   widget_destroy(parent);            // will also destroy child
 */

/* ============================================================================
 * PATTERN 2: Caller Owns Returned Memory
 * ============================================================================
 * Function allocates memory and returns it. Caller is responsible for freeing.
 * 
 * Rules:
 * - Function name should imply allocation (create, alloc, dup, copy)
 * - Function documentation MUST state caller owns memory
 * - Caller MUST free the returned memory
 * 
 * Example:
 *   char* text = strdup("Hello");     // caller owns
 *   use_text(text);
 *   free(text);                        // caller must free
 * 
 * Function documentation template:
 *   @return Newly allocated object (caller owns)
 */

/* ============================================================================
 * PATTERN 3: Callee Owns Copy
 * ============================================================================
 * Function makes a copy of input data and owns the copy.
 * Original data remains owned by caller.
 * 
 * Rules:
 * - Function copies data using malloc + memcpy or strdup
 * - Function stores the copy internally
 * - Function's destroy/cleanup frees the copy
 * - Caller can free original after function returns
 * 
 * Example:
 *   state_store_set(store, "key", data, size);  // stores copy
 *   free(data);                                 // caller can free original
 * 
 * Function documentation template:
 *   @param data Data to store (copies)
 */

/* ============================================================================
 * PATTERN 4: Borrowed Reference
 * ============================================================================
 * Function receives pointer that it may use but not store or free.
 * Pointer is only valid for the duration of the function call.
 * 
 * Rules:
 * - Function MUST NOT store the pointer
 * - Function MUST NOT free the pointer
 * - Function MUST NOT use pointer after returning
 * - Common for callbacks and temporary access
 * 
 * Example:
 *   void event_handler(const EventData* data, void* context) {
 *       // data is borrowed - valid only during this call
 *       process_event(data);  // OK
 *       store_data = data;    // WRONG - cannot store
 *       free(data);          // WRONG - cannot free
 *   }
 * 
 * Function documentation template:
 *   @param data Event data (borrowed reference, do not store or free)
 */

/* ============================================================================
 * PATTERN 5: Static/Global Lifetime
 * ============================================================================
 * Memory with program lifetime - allocated once, never freed.
 * 
 * Rules:
 * - Allocated during initialization
 * - Lives for entire program execution
 * - May be freed in cleanup function or left for OS
 * - Document as "program lifetime" in comments
 * 
 * Example:
 *   static ConfigManager* g_config = NULL;
 *   void config_init() {
 *       g_config = config_manager_create();  // program lifetime
 *   }
 */

/* ============================================================================
 * STANDARD ALLOCATION MACROS
 * ============================================================================
 * Use these macros for consistent allocation patterns.
 */

#include <stdlib.h>
#include <string.h>

// Allocate single object, zero-initialized
#define PK_ALLOC(type) \
    ((type*)calloc(1, sizeof(type)))

// Allocate array of objects, zero-initialized
#define PK_ALLOC_N(type, n) \
    ((type*)calloc(n, sizeof(type)))

// Free pointer and set to NULL (prevents use-after-free)
#define PK_FREE(ptr) \
    do { \
        free(ptr); \
        (ptr) = NULL; \
    } while(0)

// Duplicate string (caller owns)
#define PK_STRDUP(str) \
    ((str) ? strdup(str) : NULL)

// Safe string copy with null termination
#define PK_STRNCPY(dst, src, size) \
    do { \
        strncpy(dst, src, size - 1); \
        dst[size - 1] = '\0'; \
    } while(0)

/* ============================================================================
 * DOCUMENTATION REQUIREMENTS
 * ============================================================================
 * Every function that allocates memory MUST document ownership:
 * 
 * 1. For parameters:
 *    - @param foo Data to process (borrows reference)
 *    - @param bar String to store (copies string)
 *    - @param baz Buffer for result (caller provides)
 * 
 * 2. For return values:
 *    - @return New widget (caller owns)
 *    - @return Internal buffer (borrowed reference, do not free)
 *    - @return Status string (static, do not free)
 * 
 * 3. For struct members:
 *    struct Example {
 *        char* name;      // Owned, freed in destroy()
 *        Widget* parent;  // Borrowed reference, do not free
 *        int* data;       // Owned array, freed in destroy()
 *    };
 */

/* ============================================================================
 * COMMON PITFALLS TO AVOID
 * ============================================================================
 * 1. Storing borrowed references:
 *    WRONG: self->data = event_data;  // event_data is borrowed!
 *    RIGHT: self->data = malloc(size); memcpy(self->data, event_data, size);
 * 
 * 2. Double-free:
 *    WRONG: free(ptr); ... free(ptr);  // double-free!
 *    RIGHT: PK_FREE(ptr);              // sets to NULL after free
 * 
 * 3. Freeing borrowed references:
 *    WRONG: free(parent);              // parent is borrowed!
 *    RIGHT: // do nothing - borrower doesn't free
 * 
 * 4. Leaking on error paths:
 *    WRONG: obj = malloc(); if (error) return NULL;  // leak!
 *    RIGHT: obj = malloc(); if (error) { free(obj); return NULL; }
 * 
 * 5. Using after free:
 *    WRONG: free(obj); obj->field = 5;  // use-after-free!
 *    RIGHT: PK_FREE(obj);               // can't use - it's NULL
 */

/* ============================================================================
 * ERROR HANDLING PATTERNS
 * ============================================================================
 * Consistent error handling patterns used throughout PanelKit.
 * 
 * Pattern 1: Functions Returning Pointers
 * - Return NULL on error
 * - Call pk_set_last_error() before returning NULL
 * - Caller can use pk_get_last_error() for details
 * Example:
 *   Widget* widget = widget_create("button", WIDGET_BUTTON);
 *   if (!widget) {
 *       PkError err = pk_get_last_error();
 *       log_error("Failed to create widget: %s", pk_error_string(err));
 *   }
 * 
 * Pattern 2: Functions Returning bool
 * - Return false on error, true on success
 * - Call pk_set_last_error() before returning false
 * Example:
 *   if (!widget_add_child(parent, child)) {
 *       PkError err = pk_get_last_error();
 *       // Handle error
 *   }
 * 
 * Pattern 3: Void Functions (Setters)
 * - Check parameters and return early if invalid
 * - No error reporting for simple setters
 * - Log errors for operations that might fail
 * Example:
 *   void widget_set_visible(Widget* widget, bool visible) {
 *       if (!widget) return;  // Silent fail is OK for setters
 *       widget->visible = visible;
 *   }
 * 
 * Pattern 4: Functions Returning Error Codes
 * - Return PkError directly
 * - PK_OK (0) for success, negative for errors
 * - No need to call pk_set_last_error()
 * Example:
 *   PkError config_load(const char* path) {
 *       if (!path) return PK_ERROR_NULL_PARAM;
 *       // ... load config ...
 *       return PK_OK;
 *   }
 * 
 * Pattern 5: Async Operations with Callbacks
 * - Pass error code to callback
 * - Include error in callback signature
 * Example:
 *   typedef void (*async_callback)(PkError error, void* result, void* context);
 */

#endif // MEMORY_PATTERNS_H