#include "gestures.h"
#include "../core/logger.h"
#include <math.h>
#include <stdlib.h>

// Global gesture state - exact same as in app.c
static GestureState current_gesture = GESTURE_NONE;
static int gesture_button = -1;        // Button involved in current gesture, if any
static int gesture_page = -1;          // Page involved in current gesture, if any
static Uint32 gesture_start_time = 0;  // When the gesture started
static int gesture_start_x = 0;        // Starting X position
static int gesture_start_y = 0;        // Starting Y position
static int last_mouse_x = 0;           // Last mouse X for delta calculations
static int last_mouse_y = 0;           // Last mouse Y for delta calculations

// Current page context
static int current_page = 0;

// Callbacks
static gesture_click_callback g_click_callback = NULL;
static gesture_drag_callback g_drag_callback = NULL;
static gesture_swipe_callback g_swipe_callback = NULL;
static gesture_hit_test_callback g_hit_test_callback = NULL;

// Initialize gesture system
void gestures_init(
    gesture_click_callback click_cb,
    gesture_drag_callback drag_cb,
    gesture_swipe_callback swipe_cb,
    gesture_hit_test_callback hit_test_cb
) {
    g_click_callback = click_cb;
    g_drag_callback = drag_cb;
    g_swipe_callback = swipe_cb;
    g_hit_test_callback = hit_test_cb;
}

// Set current page for gesture context
void gestures_set_current_page(int page) {
    current_page = page;
}

// Begin a new gesture - exact implementation from app.c
void begin_gesture(int x, int y, int button_index) {
    current_gesture = GESTURE_POTENTIAL;
    gesture_button = button_index;
    gesture_start_time = SDL_GetTicks();
    gesture_start_x = x;
    gesture_start_y = y;
    last_mouse_x = x;
    last_mouse_y = y;
    
    log_debug("Gesture started at (%d, %d), button: %d", x, y, button_index);
}

// Update an ongoing gesture - exact implementation from app.c
void update_gesture(int x, int y) {
    // Only update if there's an active gesture
    if (current_gesture == GESTURE_NONE) {
        return;
    }
    
    // Calculate distance from start position
    int distance_x = x - gesture_start_x;
    int distance_y = y - gesture_start_y;
    float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
    
    // If we're in the potential state and exceed the drag threshold,
    // determine if this is a horizontal or vertical drag
    if (current_gesture == GESTURE_POTENTIAL && distance > DRAG_THRESHOLD_PX) {
        if (abs(distance_x) > abs(distance_y)) {
            // Horizontal drag - page swipe
            current_gesture = GESTURE_DRAG_HORZ;
            log_debug("Gesture: horizontal drag detected (%.1f px)", distance);
        } else {
            // Vertical drag - content scroll
            current_gesture = GESTURE_DRAG_VERT;
            log_debug("Gesture: vertical drag detected (%.1f px)", distance);
        }
    }
    
    // Process based on gesture type
    if (current_gesture == GESTURE_DRAG_VERT) {
        // Vertical dragging - notify via callback
        int delta_y = last_mouse_y - y;
        
        if (g_drag_callback) {
            g_drag_callback(0, delta_y, false);
        }
    }
    else if (current_gesture == GESTURE_DRAG_HORZ) {
        // Horizontal dragging - page swiping
        int delta_x = x - gesture_start_x;
        
        if (g_swipe_callback) {
            g_swipe_callback((float)delta_x, false); // false = not complete yet
        }
        
        if (g_drag_callback) {
            g_drag_callback(delta_x, 0, true);
        }
    }
    
    // Update last position
    last_mouse_x = x;
    last_mouse_y = y;
}

// End the current gesture - exact implementation from app.c
void end_gesture(int x, int y) {
    // Only process if there's an active gesture
    if (current_gesture == GESTURE_NONE) {
        return;
    }
    
    // Calculate time elapsed and distance
    Uint32 time_elapsed = SDL_GetTicks() - gesture_start_time;
    int distance_x = x - gesture_start_x;
    int distance_y = y - gesture_start_y;
    float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
    
    log_debug("Gesture ended: %s, time: %dms, distance: %.1fpx", 
           current_gesture == GESTURE_POTENTIAL ? "POTENTIAL" :
           current_gesture == GESTURE_DRAG_VERT ? "DRAG_VERT" :
           current_gesture == GESTURE_DRAG_HORZ ? "DRAG_HORZ" :
           current_gesture == GESTURE_HOLD ? "HOLD" : "UNKNOWN",
           time_elapsed, distance);
    
    // If this was a potential gesture that didn't exceed thresholds,
    // it's a click
    if (current_gesture == GESTURE_POTENTIAL && 
        time_elapsed < CLICK_TIMEOUT_MS && 
        distance < DRAG_THRESHOLD_PX) {
        
        current_gesture = GESTURE_CLICK;
        
        // Handle button click if applicable
        if (gesture_button >= 0 && gesture_page == current_page && g_click_callback) {
            g_click_callback(gesture_button);
        }
    }
    
    // Handle the end of a horizontal drag (page swipe)
    if (current_gesture == GESTURE_DRAG_HORZ && g_swipe_callback) {
        float distance_x_float = (float)(x - gesture_start_x);
        g_swipe_callback(distance_x_float, true); // true = complete
    }
    
    // Reset gesture state
    current_gesture = GESTURE_NONE;
    gesture_button = -1;
}

// Cancel the current gesture - exact implementation from app.c
void cancel_gesture() {
    if (current_gesture != GESTURE_NONE) {
        log_debug("Gesture cancelled");
        current_gesture = GESTURE_NONE;
        gesture_button = -1;
    }
}

// Unified touch event handlers - exact implementation from app.c
void handle_touch_down(int x, int y, const char* source) {
    // Get button under touch (if any)
    int button_index = -1;
    if (g_hit_test_callback) {
        button_index = g_hit_test_callback(x, y, current_page);
    }
    
    // Start a new gesture
    begin_gesture(x, y, button_index);
    gesture_page = current_page;
    
    log_debug("Touch DOWN from %s at (%d,%d), button=%d", source, x, y, button_index);
}

void handle_touch_up(int x, int y, const char* source) {
    // End the current gesture (if any)
    end_gesture(x, y);
    log_debug("Touch UP from %s at (%d,%d)", source, x, y);
}

void handle_touch_motion(int x, int y, const char* source) {
    // Update current gesture (if any)
    update_gesture(x, y);
    // Note: motion logging is verbose, so only log at trace level if needed
}

// State getters
GestureState gestures_get_state(void) {
    return current_gesture;
}

int gestures_get_button(void) {
    return gesture_button;
}

int gestures_get_page(void) {
    return gesture_page;
}

bool gestures_is_dragging_page(void) {
    return current_gesture == GESTURE_DRAG_HORZ;
}