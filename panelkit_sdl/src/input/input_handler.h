/**
 * @file input_handler.h
 * @brief Input abstraction interface for touch/mouse/keyboard input
 * 
 * This module provides a clean abstraction over different input sources,
 * allowing backends to provide input through various mechanisms:
 * - SDL's built-in event system (for desktop/KMSDRM)
 * - Manual Linux input device reading (for headless/offscreen SDL)
 * - Mock/test input sources
 * 
 * Design principles:
 * - Complete separation from display backend (no coupling)
 * - Pluggable input sources via strategy pattern
 * - Thread-safe event delivery
 * - Extensible for future input types
 */

#ifndef PANELKIT_INPUT_HANDLER_H
#define PANELKIT_INPUT_HANDLER_H

#include "../core/sdl_includes.h"
#include <stdbool.h>

/* Forward declarations */
typedef struct InputHandler InputHandler;
typedef struct InputSource InputSource;

/* Input source types */
typedef enum {
    INPUT_SOURCE_SDL_NATIVE,    /* SDL's built-in event system */
    INPUT_SOURCE_LINUX_EVDEV,   /* Direct Linux /dev/input/event* reading */
    INPUT_SOURCE_MOCK,          /* Mock input for testing */
    INPUT_SOURCE_CUSTOM         /* User-provided custom source */
} InputSourceType;

/* Input device capabilities */
typedef struct {
    bool has_touch;
    bool has_mouse;
    bool has_keyboard;
    int max_touch_points;
    int touch_x_min, touch_x_max;
    int touch_y_min, touch_y_max;
} InputCapabilities;

/* Input configuration */
typedef struct {
    InputSourceType source_type;
    const char* device_path;     /* For LINUX_EVDEV: specific device path (NULL for auto-detect) */
    bool auto_detect_devices;    /* Auto-detect input devices */
    bool enable_mouse_emulation; /* Emulate mouse from touch events */
    int reconnect_attempts;      /* Number of reconnection attempts on device loss (0 = disabled) */
    int reconnect_delay_ms;      /* Delay between reconnection attempts in milliseconds */
} InputConfig;

/* Forward declarations for implementation types */
struct SDLNativeData;
struct EvdevData;
struct MockData;

/**
 * Input source interface (Strategy pattern)
 * Implementations provide specific input reading mechanisms
 */
struct InputSource {
    /* Source type identifier */
    InputSourceType type;
    
    /* Source name for logging */
    const char* name;
    
    /* Type-safe implementation data */
    union {
        struct SDLNativeData* sdl;
        struct EvdevData* evdev;
        struct MockData* mock;
        void* custom;  /* For custom sources */
    } impl;
    
    /* Initialize the input source
     * @param source The input source instance
     * @param config Input configuration
     * @return true on success, false on failure
     */
    bool (*initialize)(InputSource* source, const InputConfig* config);
    
    /* Start input processing (may spawn threads)
     * @param source The input source instance
     * @param handler Parent handler for event delivery
     * @return true on success, false on failure
     */
    bool (*start)(InputSource* source, InputHandler* handler);
    
    /* Stop input processing
     * @param source The input source instance
     */
    void (*stop)(InputSource* source);
    
    /* Get input capabilities
     * @param source The input source instance
     * @param caps Output capabilities structure
     * @return true if capabilities retrieved, false otherwise
     */
    bool (*get_capabilities)(InputSource* source, InputCapabilities* caps);
    
    /* Cleanup and destroy the source
     * @param source The input source instance
     */
    void (*cleanup)(InputSource* source);
};

/**
 * Main input handler interface
 * Manages input sources and delivers events to SDL
 */
struct InputHandler {
    /* Active input source */
    InputSource* source;
    
    /* Configuration */
    InputConfig config;
    
    /* State */
    bool running;
    
    /* Statistics */
    struct {
        uint64_t events_processed;
        uint64_t touch_events;
        uint64_t mouse_events;
        uint64_t keyboard_events;
    } stats;
};

/**
 * Create input handler with configuration.
 * 
 * @param config Input configuration (required)
 * @return New handler or NULL on error (caller owns)
 * @note Handler must be started before processing input
 */
InputHandler* input_handler_create(const InputConfig* config);

/**
 * Start input processing.
 * 
 * @param handler Input handler (required)
 * @return true on success, false on error
 * @note May spawn background threads for input sources
 */
bool input_handler_start(InputHandler* handler);

/**
 * Stop input processing.
 * 
 * @param handler Input handler (required)
 * @note Stops all background threads and sources
 */
void input_handler_stop(InputHandler* handler);

/**
 * Get input device capabilities.
 * 
 * @param handler Input handler (required)
 * @param caps Receives capabilities (required)
 * @return true if capabilities retrieved, false otherwise
 */
bool input_handler_get_capabilities(InputHandler* handler, InputCapabilities* caps);

/**
 * Get input processing statistics.
 * 
 * @param handler Input handler (required)
 * @return Read-only statistics pointer (never NULL)
 * @note Statistics are owned by handler - do not free
 */
const struct InputHandler_stats* input_handler_get_stats(InputHandler* handler);

/**
 * Push SDL event from input source.
 * 
 * @param handler Input handler (required)
 * @param event SDL event to deliver (required)
 * @return true if pushed, false if queue full
 * @note Thread-safe - can be called from source threads
 */
bool input_handler_push_event(InputHandler* handler, SDL_Event* event);

/**
 * Destroy input handler.
 * 
 * @param handler Handler to destroy (can be NULL)
 * @note Stops processing before destruction
 */
void input_handler_destroy(InputHandler* handler);

// Factory functions for built-in input sources

/**
 * Create SDL native input source.
 * 
 * @return New source or NULL on error (caller owns)
 * @note Uses SDL's built-in event system
 */
InputSource* input_source_sdl_native_create(void);

/**
 * Create Linux evdev input source.
 * 
 * @return New source or NULL on error (caller owns)
 * @note Reads directly from /dev/input/event* devices
 */
InputSource* input_source_linux_evdev_create(void);

/**
 * Create mock input source for testing.
 * 
 * @return New source or NULL on error (caller owns)
 * @note Generates synthetic input events
 */
InputSource* input_source_mock_create(void);

// Mock input source control API

/**
 * Queue a single event in mock source.
 * 
 * @param source Mock input source (required)
 * @param event Event to queue (required, copied)
 */
void input_mock_queue_event(InputSource* source, SDL_Event* event);

/**
 * Start generating a pattern in mock source.
 * 
 * @param source Mock input source (required)
 * @param pattern Pattern type (MOCK_PATTERN_*)
 */
void input_mock_start_pattern(InputSource* source, int pattern);

/**
 * Stop pattern generation in mock source.
 * 
 * @param source Mock input source (required)
 */
void input_mock_stop_pattern(InputSource* source);

/**
 * Configure pattern timing in mock source.
 * 
 * @param source Mock input source (required)
 * @param delay_ms Delay between events in milliseconds
 * @param duration_ms Total pattern duration in milliseconds
 */
void input_mock_configure_pattern(InputSource* source, int delay_ms, int duration_ms);

/** Mock pattern types */
#define MOCK_PATTERN_NONE   0  /**< No pattern */
#define MOCK_PATTERN_TAP    1  /**< Tap gestures */
#define MOCK_PATTERN_SWIPE  2  /**< Swipe gestures */
#define MOCK_PATTERN_PINCH  3  /**< Pinch gestures */
#define MOCK_PATTERN_CIRCLE 4  /**< Circular gestures */

#endif /* PANELKIT_INPUT_HANDLER_H */