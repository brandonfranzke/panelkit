/**
 * @file display_backend.c
 * @brief Display backend abstraction implementation
 */

#include "display_backend.h"
#include "../core/logger.h"
#include "../core/error.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations for backend implementations */
extern DisplayBackend* display_backend_sdl_create(const DisplayConfig* config);
extern DisplayBackend* display_backend_sdl_drm_create(const DisplayConfig* config);

/* Get backend type name */
const char* display_backend_type_name(DisplayBackendType type) {
    switch (type) {
        case DISPLAY_BACKEND_SDL:
            return "SDL";
        case DISPLAY_BACKEND_SDL_DRM:
            return "SDL+DRM";
        case DISPLAY_BACKEND_AUTO:
            return "Auto";
        default:
            return "Unknown";
    }
}

/* Check backend availability */
bool display_backend_available(DisplayBackendType type) {
    switch (type) {
        case DISPLAY_BACKEND_SDL:
            /* Standard SDL is always available if SDL is compiled */
            return true;
            
        case DISPLAY_BACKEND_SDL_DRM:
            /* SDL+DRM is only available on Linux with DRM support */
#ifdef __linux__
            /* Could add more checks here for DRM device availability */
            return true;
#else
            return false;
#endif
            
        case DISPLAY_BACKEND_AUTO:
            /* Auto is always "available" as it will pick something */
            return true;
            
        default:
            return false;
    }
}

/* Auto-detect best backend */
static DisplayBackendType detect_best_backend(void) {
    /* Check environment variable first */
    const char* backend_env = getenv("PANELKIT_DISPLAY_BACKEND");
    if (backend_env) {
        if (strcmp(backend_env, "sdl") == 0) {
            log_info("Display backend forced to SDL via environment");
            return DISPLAY_BACKEND_SDL;
        } else if (strcmp(backend_env, "sdl_drm") == 0) {
            log_info("Display backend forced to SDL+DRM via environment");
            return DISPLAY_BACKEND_SDL_DRM;
        }
    }
    
    /* Auto-detection logic */
#ifdef __linux__
    /* On Linux, check if we're running without a display server */
    const char* display = getenv("DISPLAY");
    const char* wayland = getenv("WAYLAND_DISPLAY");
    
    if (!display && !wayland) {
        /* No display server, likely embedded - use DRM if available */
        if (display_backend_available(DISPLAY_BACKEND_SDL_DRM)) {
            log_info("Auto-detected embedded environment, using SDL+DRM");
            return DISPLAY_BACKEND_SDL_DRM;
        }
    }
#endif
    
    /* Default to standard SDL */
    log_info("Auto-detected desktop environment, using standard SDL");
    return DISPLAY_BACKEND_SDL;
}

/* Create display backend */
DisplayBackend* display_backend_create(const DisplayConfig* config) {
    if (!config) {
        log_error("Display backend creation failed: NULL config");
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "display_backend_create: config is NULL");
        return NULL;
    }
    
    DisplayBackendType type = config->backend_type;
    
    /* Handle auto-detection */
    if (type == DISPLAY_BACKEND_AUTO) {
        type = detect_best_backend();
    }
    
    /* Check availability */
    if (!display_backend_available(type)) {
        log_error("Display backend %s is not available on this platform",
                  display_backend_type_name(type));
        pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
            "display_backend_create: Backend %s not available on this platform",
            display_backend_type_name(type));
        return NULL;
    }
    
    /* Create the appropriate backend */
    DisplayBackend* backend = NULL;
    
    switch (type) {
        case DISPLAY_BACKEND_SDL:
            backend = display_backend_sdl_create(config);
            break;
            
        case DISPLAY_BACKEND_SDL_DRM:
            backend = display_backend_sdl_drm_create(config);
            break;
            
        default:
            log_error("Unknown display backend type: %d", type);
            pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                "display_backend_create: Unknown backend type %d", type);
            return NULL;
    }
    
    if (backend) {
        log_info("Display backend created: %s (%dx%d)",
                 backend->name,
                 backend->actual_width,
                 backend->actual_height);
    } else {
        /* Error context already set by backend-specific create function */
    }
    
    return backend;
}

/* Present frame */
void display_backend_present(DisplayBackend* backend) {
    if (!backend) {
        return;
    }
    
    if (backend->present) {
        backend->present(backend);
    }
}

/* Destroy backend */
void display_backend_destroy(DisplayBackend* backend) {
    if (!backend) {
        return;
    }
    
    log_info("Destroying display backend: %s", backend->name);
    
    if (backend->cleanup) {
        backend->cleanup(backend);
    }
    
    free(backend);
}