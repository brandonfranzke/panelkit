/**
 * @file backend_sdl.c
 * @brief Standard SDL display backend implementation
 */

#include "display_backend.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>

/* Standard SDL backend implementation data */
typedef struct {
    /* SDL owns the window and renderer, we just reference them */
    bool owns_sdl_init;
} SDLBackendImpl;

/* Cleanup function */
static void sdl_backend_cleanup(DisplayBackend* backend) {
    if (!backend || !backend->impl) {
        return;
    }
    
    SDLBackendImpl* impl = (SDLBackendImpl*)backend->impl;
    
    /* Destroy SDL resources if we own them */
    if (backend->renderer) {
        SDL_DestroyRenderer(backend->renderer);
        backend->renderer = NULL;
    }
    
    if (backend->window) {
        SDL_DestroyWindow(backend->window);
        backend->window = NULL;
    }
    
    /* Quit SDL if we initialized it */
    if (impl->owns_sdl_init) {
        SDL_Quit();
    }
    
    free(impl);
    backend->impl = NULL;
}

/* Present function - standard SDL doesn't need extra work */
static void sdl_backend_present(DisplayBackend* backend) {
    (void)backend;
    /* SDL_RenderPresent already handles everything */
}

/* Set vsync */
static bool sdl_backend_set_vsync(DisplayBackend* backend, bool enable) {
    (void)enable; /* Not used - vsync is set at creation time */
    
    if (!backend || !backend->renderer) {
        return false;
    }
    
    /* SDL vsync is controlled at renderer creation time */
    log_warn("SDL backend: vsync can only be set at creation time");
    return false;
}

/* Set fullscreen */
static bool sdl_backend_set_fullscreen(DisplayBackend* backend, bool enable) {
    if (!backend || !backend->window) {
        return false;
    }
    
    Uint32 flags = enable ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    if (SDL_SetWindowFullscreen(backend->window, flags) != 0) {
        LOG_SDL_ERROR("Failed to set fullscreen");
        return false;
    }
    
    log_info("Fullscreen mode: %s", enable ? "enabled" : "disabled");
    return true;
}

/* Create standard SDL backend */
DisplayBackend* display_backend_sdl_create(const DisplayConfig* config) {
    /* Allocate backend structure */
    DisplayBackend* backend = calloc(1, sizeof(DisplayBackend));
    if (!backend) {
        log_error("Failed to allocate display backend");
        return NULL;
    }
    
    /* Allocate implementation data */
    SDLBackendImpl* impl = calloc(1, sizeof(SDLBackendImpl));
    if (!impl) {
        log_error("Failed to allocate SDL backend implementation");
        free(backend);
        return NULL;
    }
    
    /* Set up backend structure */
    backend->type = DISPLAY_BACKEND_SDL;
    backend->name = "Standard SDL";
    backend->impl = impl;
    backend->present = sdl_backend_present;
    backend->cleanup = sdl_backend_cleanup;
    backend->set_vsync = sdl_backend_set_vsync;
    backend->set_fullscreen = sdl_backend_set_fullscreen;
    
    /* Initialize SDL if needed */
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            LOG_SDL_ERROR("Failed to initialize SDL");
            free(impl);
            free(backend);
            return NULL;
        }
        impl->owns_sdl_init = true;
        log_info("SDL video subsystem initialized by backend");
    }
    
    /* Create window */
    Uint32 window_flags = SDL_WINDOW_SHOWN;
    if (config->fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    
    backend->window = SDL_CreateWindow(
        config->title ? config->title : "PanelKit",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config->width,
        config->height,
        window_flags
    );
    
    if (!backend->window) {
        LOG_SDL_ERROR("Failed to create window");
        sdl_backend_cleanup(backend);
        free(backend);
        return NULL;
    }
    
    /* Create renderer */
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if (config->vsync) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    backend->renderer = SDL_CreateRenderer(backend->window, -1, renderer_flags);
    if (!backend->renderer) {
        LOG_SDL_ERROR("Failed to create renderer");
        sdl_backend_cleanup(backend);
        free(backend);
        return NULL;
    }
    
    /* Get actual window size (may differ in fullscreen) */
    SDL_GetWindowSize(backend->window, &backend->actual_width, &backend->actual_height);
    
    /* Log renderer info */
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(backend->renderer, &info) == 0) {
        log_info("SDL Renderer: %s", info.name);
        log_debug("  Max texture size: %dx%d", info.max_texture_width, info.max_texture_height);
        log_debug("  Flags: %s %s",
                  (info.flags & SDL_RENDERER_SOFTWARE) ? "SOFTWARE" : "HARDWARE",
                  (info.flags & SDL_RENDERER_PRESENTVSYNC) ? "VSYNC" : "NO-VSYNC");
    }
    
    return backend;
}