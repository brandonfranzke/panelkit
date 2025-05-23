/**
 * @file display_backend.h
 * @brief Display backend abstraction interface
 * 
 * This module provides a clean abstraction over different display backends,
 * allowing the application to remain agnostic about whether it's using
 * standard SDL, SDL+DRM, or future display technologies.
 */

#ifndef PANELKIT_DISPLAY_BACKEND_H
#define PANELKIT_DISPLAY_BACKEND_H

#include <SDL.h>
#include <stdbool.h>

/* Forward declaration */
typedef struct DisplayBackend DisplayBackend;

/* Display backend types */
typedef enum {
    DISPLAY_BACKEND_SDL,        /* Standard SDL with window manager */
    DISPLAY_BACKEND_SDL_DRM,    /* SDL + Direct DRM for embedded */
    DISPLAY_BACKEND_AUTO        /* Auto-detect best backend */
} DisplayBackendType;

/* Display configuration */
typedef struct {
    int width;
    int height;
    const char* title;
    DisplayBackendType backend_type;
    bool fullscreen;
    bool vsync;
} DisplayConfig;

/* Display backend interface */
struct DisplayBackend {
    /* Backend type identifier */
    DisplayBackendType type;
    
    /* Backend name for logging */
    const char* name;
    
    /* Private implementation data */
    void* impl;
    
    /* SDL window handle (may be NULL for some backends) */
    SDL_Window* window;
    
    /* SDL renderer handle */
    SDL_Renderer* renderer;
    
    /* Actual display dimensions (may differ from requested) */
    int actual_width;
    int actual_height;
    
    /* Backend operations */
    void (*present)(DisplayBackend* backend);
    void (*cleanup)(DisplayBackend* backend);
    
    /* Optional operations */
    bool (*set_vsync)(DisplayBackend* backend, bool enable);
    bool (*set_fullscreen)(DisplayBackend* backend, bool enable);
};

/* Initialize display backend with configuration
 * @param config Display configuration
 * @return Backend instance or NULL on failure
 */
DisplayBackend* display_backend_create(const DisplayConfig* config);

/* Present the current frame
 * @param backend Display backend instance
 * 
 * This should be called after SDL_RenderPresent() to ensure
 * the frame is actually displayed on hardware.
 */
void display_backend_present(DisplayBackend* backend);

/* Cleanup and destroy backend
 * @param backend Display backend instance
 */
void display_backend_destroy(DisplayBackend* backend);

/* Get backend type name for logging
 * @param type Backend type
 * @return String representation of backend type
 */
const char* display_backend_type_name(DisplayBackendType type);

/* Check if a backend type is available
 * @param type Backend type to check
 * @return true if backend is available
 */
bool display_backend_available(DisplayBackendType type);

#endif /* PANELKIT_DISPLAY_BACKEND_H */