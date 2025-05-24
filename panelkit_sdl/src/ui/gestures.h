#ifndef GESTURES_H
#define GESTURES_H

#include "../core/sdl_includes.h"
#include <stdbool.h>

// Gesture thresholds - exact values from app.c
#define CLICK_TIMEOUT_MS 300         // Time in ms to consider a press a "click" vs "hold"
#define DRAG_THRESHOLD_PX 10         // Minimum movement in pixels to consider a drag
#define HOLD_THRESHOLD_MS 500        // Time in ms to consider a press a "hold" vs "click"
#define PAGE_SWIPE_THRESHOLD_PX 100  // Minimum horizontal movement to trigger page change

// Gesture states - exact enum from app.c
typedef enum {
    GESTURE_NONE,         // No gesture in progress
    GESTURE_POTENTIAL,    // Press detected, waiting to determine gesture type
    GESTURE_CLICK,        // Quick press and release (under thresholds)
    GESTURE_DRAG_VERT,    // Vertical dragging (scrolling)
    GESTURE_DRAG_HORZ,    // Horizontal dragging (page swiping)
    GESTURE_HOLD          // Press and hold (over time threshold)
} GestureState;

// Callback function types
typedef void (*gesture_click_callback)(int button_index);
typedef void (*gesture_drag_callback)(int delta_x, int delta_y, bool is_horizontal);
typedef void (*gesture_swipe_callback)(float offset, bool is_complete);
typedef int (*gesture_hit_test_callback)(int x, int y, int page_index);

// Initialize gesture system with callbacks
void gestures_init(
    gesture_click_callback click_cb,
    gesture_drag_callback drag_cb,
    gesture_swipe_callback swipe_cb,
    gesture_hit_test_callback hit_test_cb
);

// Core gesture functions - exact signatures from app.c
void begin_gesture(int x, int y, int button_index);
void update_gesture(int x, int y);
void end_gesture(int x, int y);
void cancel_gesture();

// Touch event handlers - exact signatures from app.c
void handle_touch_down(int x, int y, const char* source);
void handle_touch_up(int x, int y, const char* source);
void handle_touch_motion(int x, int y, const char* source);

// State getters
GestureState gestures_get_state(void);
int gestures_get_button(void);
int gestures_get_page(void);
bool gestures_is_dragging_page(void);

// Set current page for gesture context
void gestures_set_current_page(int page);

#endif // GESTURES_H