/**
 * @file input_source_sdl.c
 * @brief SDL native input source implementation
 * 
 * Uses SDL's built-in event system for input. This works when SDL's
 * video driver supports input (e.g., X11, Wayland, KMSDRM) but not
 * with offscreen or dummy drivers.
 */

#include "input_handler.h"
#include "../core/logger.h"
#include <stdlib.h>

/* Private implementation data */
typedef struct {
    InputHandler* handler;
    int num_touch_devices;
} SDLNativeData;

/* Initialize the input source */
static bool sdl_initialize(InputSource* source, const InputConfig* config) {
    (void)config; /* Unused for SDL native */
    SDLNativeData* data = (SDLNativeData*)source->impl;
    
    /* Check SDL video subsystem is initialized */
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        log_error("SDL video subsystem not initialized");
        return false;
    }
    
    /* Count touch devices */
    data->num_touch_devices = SDL_GetNumTouchDevices();
    log_info("SDL reports %d touch device(s)", data->num_touch_devices);
    
    for (int i = 0; i < data->num_touch_devices; i++) {
        SDL_TouchID id = SDL_GetTouchDevice(i);
        log_info("  Touch device %d: ID=%lld", i, (long long)id);
    }
    
    return true;
}

/* Start input processing */
static bool sdl_start(InputSource* source, InputHandler* handler) {
    SDLNativeData* data = (SDLNativeData*)source->impl;
    data->handler = handler;
    
    /* SDL native doesn't need a separate thread - events come through SDL_PollEvent */
    log_info("SDL native input source started");
    return true;
}

/* Stop input processing */
static void sdl_stop(InputSource* source) {
    (void)source; /* Nothing to stop for SDL native */
    log_info("SDL native input source stopped");
}

/* Get input capabilities */
static bool sdl_get_capabilities(InputSource* source, InputCapabilities* caps) {
    SDLNativeData* data = (SDLNativeData*)source->impl;
    
    if (!caps) {
        return false;
    }
    
    /* SDL native can potentially support all input types */
    caps->has_touch = data->num_touch_devices > 0;
    caps->has_mouse = true;  /* SDL always supports mouse */
    caps->has_keyboard = true; /* SDL always supports keyboard */
    caps->max_touch_points = 10; /* SDL doesn't report this, use reasonable default */
    
    /* SDL doesn't provide touch device range info */
    caps->touch_x_min = 0;
    caps->touch_x_max = 0;
    caps->touch_y_min = 0;
    caps->touch_y_max = 0;
    
    return true;
}

/* Cleanup and destroy */
static void sdl_cleanup(InputSource* source) {
    SDLNativeData* data = (SDLNativeData*)source->impl;
    
    /* Free implementation data */
    free(data);
    source->impl = NULL;
}

/* Create SDL native input source */
InputSource* input_source_sdl_native_create(void) {
    InputSource* source = calloc(1, sizeof(InputSource));
    if (!source) {
        return NULL;
    }
    
    SDLNativeData* data = calloc(1, sizeof(SDLNativeData));
    if (!data) {
        free(source);
        return NULL;
    }
    
    /* Initialize structure */
    source->type = INPUT_SOURCE_SDL_NATIVE;
    source->name = "SDL Native";
    source->impl = data;
    source->initialize = sdl_initialize;
    source->start = sdl_start;
    source->stop = sdl_stop;
    source->get_capabilities = sdl_get_capabilities;
    source->cleanup = sdl_cleanup;
    
    return source;
}