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

/* Create input handler with configuration
 * @param config Input configuration
 * @return Handler instance or NULL on failure
 */
InputHandler* input_handler_create(const InputConfig* config);

/* Start input processing
 * @param handler Input handler instance
 * @return true on success, false on failure
 */
bool input_handler_start(InputHandler* handler);

/* Stop input processing
 * @param handler Input handler instance
 */
void input_handler_stop(InputHandler* handler);

/* Get input capabilities
 * @param handler Input handler instance
 * @param caps Output capabilities structure
 * @return true if capabilities retrieved, false otherwise
 */
bool input_handler_get_capabilities(InputHandler* handler, InputCapabilities* caps);

/* Get input statistics
 * @param handler Input handler instance
 * @return Pointer to statistics (read-only)
 */
const struct InputHandler_stats* input_handler_get_stats(InputHandler* handler);

/* Push SDL event (for input sources to deliver events)
 * Thread-safe - can be called from input source threads
 * @param handler Input handler instance
 * @param event SDL event to push
 * @return true if event was pushed, false if queue full
 */
bool input_handler_push_event(InputHandler* handler, SDL_Event* event);

/* Cleanup and destroy handler
 * @param handler Input handler instance
 */
void input_handler_destroy(InputHandler* handler);

/* Factory functions for built-in input sources */
InputSource* input_source_sdl_native_create(void);
InputSource* input_source_linux_evdev_create(void);
InputSource* input_source_mock_create(void);

/* Mock input source control API */
void input_mock_queue_event(InputSource* source, SDL_Event* event);
void input_mock_start_pattern(InputSource* source, int pattern);
void input_mock_stop_pattern(InputSource* source);
void input_mock_configure_pattern(InputSource* source, int delay_ms, int duration_ms);

/* Mock pattern types */
#define MOCK_PATTERN_NONE   0
#define MOCK_PATTERN_TAP    1
#define MOCK_PATTERN_SWIPE  2
#define MOCK_PATTERN_PINCH  3
#define MOCK_PATTERN_CIRCLE 4

#endif /* PANELKIT_INPUT_HANDLER_H */