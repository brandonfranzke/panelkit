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

#include "core/sdl_includes.h"
#include <stdbool.h>

/** Opaque display backend handle */
typedef struct DisplayBackend DisplayBackend;

/**
 * Display backend implementation types.
 * Determines rendering and display strategy.
 */
typedef enum {
    DISPLAY_BACKEND_SDL,        /**< Standard SDL with window manager */
    DISPLAY_BACKEND_SDL_DRM,    /**< SDL + Direct DRM for embedded */
    DISPLAY_BACKEND_AUTO        /**< Auto-detect best backend */
} DisplayBackendType;

/**
 * Display backend configuration.
 * Initial settings for display creation.
 */
typedef struct {
    int width;                      /**< Requested width in pixels */
    int height;                     /**< Requested height in pixels */
    const char* title;              /**< Window title (if windowed) */
    DisplayBackendType backend_type; /**< Backend implementation to use */
    bool fullscreen;                /**< Start in fullscreen mode */
    bool vsync;                     /**< Enable vertical sync */
} DisplayConfig;

/* Forward declarations for implementation types */
struct SDLBackendImpl;
struct SDLDRMBackendImpl;

/**
 * Display backend interface.
 * Provides unified API over different display technologies.
 */
struct DisplayBackend {
    /* Backend type identifier */
    DisplayBackendType type;
    
    /* Backend name for logging */
    const char* name;
    
    /* Type-safe implementation data */
    union {
        struct SDLBackendImpl* sdl;
        struct SDLDRMBackendImpl* sdl_drm;
    } impl;
    
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

/**
 * Create a display backend with configuration.
 * 
 * @param config Display configuration (required)
 * @return New backend or NULL on error (caller owns)
 * @note Backend type AUTO will probe available backends
 */
DisplayBackend* display_backend_create(const DisplayConfig* config);

/**
 * Present the current frame to display.
 * 
 * @param backend Display backend (required)
 * @note Call after SDL_RenderPresent() to ensure hardware sync
 */
void display_backend_present(DisplayBackend* backend);

/**
 * Destroy a display backend.
 * 
 * @param backend Backend to destroy (can be NULL)
 * @note Cleans up all backend resources
 */
void display_backend_destroy(DisplayBackend* backend);

/**
 * Get human-readable name for backend type.
 * 
 * @param type Backend type enum value
 * @return Static string name (never NULL)
 */
const char* display_backend_type_name(DisplayBackendType type);

/**
 * Check if a backend type is available on this system.
 * 
 * @param type Backend type to check
 * @return true if backend can be used, false otherwise
 * @note SDL backend is always available
 */
bool display_backend_available(DisplayBackendType type);

#endif /* PANELKIT_DISPLAY_BACKEND_H */