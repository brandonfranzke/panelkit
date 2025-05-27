/**
 * @file input_handler.c
 * @brief Main input handler implementation
 * 
 * Manages input sources and provides a unified interface for input handling.
 * Supports pluggable input sources through the strategy pattern.
 */

#include "input_handler.h"
#include "../core/logger.h"
#include "../core/error.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* Thread safety for event pushing */
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Create input handler with configuration */
InputHandler* input_handler_create(const InputConfig* config) {
    if (!config) {
        log_error("Input handler creation failed: NULL config");
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM, 
            "input_handler_create: config is NULL");
        return NULL;
    }
    
    InputHandler* handler = calloc(1, sizeof(InputHandler));
    if (!handler) {
        log_error("Failed to allocate input handler");
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "input_handler_create: Failed to allocate %zu bytes", sizeof(InputHandler));
        return NULL;
    }
    
    /* Copy configuration */
    handler->config = *config;
    
    /* Create appropriate input source */
    switch (config->source_type) {
        case INPUT_SOURCE_SDL_NATIVE:
            handler->source = input_source_sdl_native_create();
            break;
            
        case INPUT_SOURCE_LINUX_EVDEV:
#ifdef __linux__
            handler->source = input_source_linux_evdev_create();
#else
            log_error("Linux evdev input source is only available on Linux");
            free(handler);
            return NULL;
#endif
            break;
            
        case INPUT_SOURCE_MOCK:
            handler->source = input_source_mock_create();
            break;
            
        default:
            log_error("Unknown input source type: %d", config->source_type);
            pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                "input_handler_create: Unknown source type %d", config->source_type);
            free(handler);
            return NULL;
    }
    
    if (!handler->source) {
        log_error("Failed to create input source");
        pk_set_last_error_with_context(PK_ERROR_INPUT_SOURCE_FAILED,
            "input_handler_create: Failed to create source type %d", config->source_type);
        free(handler);
        return NULL;
    }
    
    /* Initialize the source */
    if (!handler->source->initialize(handler->source, config)) {
        log_error("Failed to initialize input source");
        pk_set_last_error_with_context(PK_ERROR_INPUT_INIT_FAILED,
            "input_handler_create: Source '%s' initialization failed", 
            handler->source->name ? handler->source->name : "unknown");
        handler->source->cleanup(handler->source);
        free(handler->source);
        free(handler);
        return NULL;
    }
    
    log_info("Input handler created with source: %s", handler->source->name);
    return handler;
}

/* Start input processing */
bool input_handler_start(InputHandler* handler) {
    if (!handler || !handler->source) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "input_handler_start: handler=%p, source=%p", 
            (void*)handler, handler ? (void*)handler->source : NULL);
        return false;
    }
    
    if (handler->running) {
        log_warn("Input handler already running");
        return true;
    }
    
    /* Start the input source */
    if (!handler->source->start(handler->source, handler)) {
        log_error("Failed to start input source");
        pk_set_last_error_with_context(PK_ERROR_INPUT_SOURCE_FAILED,
            "input_handler_start: Source '%s' failed to start",
            handler->source->name ? handler->source->name : "unknown");
        return false;
    }
    
    handler->running = true;
    log_info("Input handler started");
    return true;
}

/* Stop input processing */
void input_handler_stop(InputHandler* handler) {
    if (!handler || !handler->running) {
        return;
    }
    
    /* Stop the input source */
    if (handler->source) {
        handler->source->stop(handler->source);
    }
    
    handler->running = false;
    log_info("Input handler stopped");
}

/* Get input capabilities */
bool input_handler_get_capabilities(InputHandler* handler, InputCapabilities* caps) {
    if (!handler || !handler->source || !caps) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "input_handler_get_capabilities: handler=%p, source=%p, caps=%p",
            (void*)handler, handler ? (void*)handler->source : NULL, (void*)caps);
        return false;
    }
    
    return handler->source->get_capabilities(handler->source, caps);
}

/* Get input statistics */
const struct InputHandler_stats* input_handler_get_stats(InputHandler* handler) {
    if (!handler) {
        return NULL;
    }
    
    return (const struct InputHandler_stats*)&handler->stats;
}

/* Push SDL event (thread-safe) */
bool input_handler_push_event(InputHandler* handler, SDL_Event* event) {
    if (!handler || !event) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "input_handler_push_event: handler=%p, event=%p",
            (void*)handler, (void*)event);
        return false;
    }
    
    /* Thread-safe SDL event push */
    pthread_mutex_lock(&event_mutex);
    int result = SDL_PushEvent(event);
    pthread_mutex_unlock(&event_mutex);
    
    if (result < 0) {
        log_error("Failed to push SDL event: %s", SDL_GetError());
        pk_set_last_error_with_context(PK_ERROR_SDL,
            "input_handler_push_event: SDL_PushEvent failed: %s", SDL_GetError());
        return false;
    }
    
    /* Update statistics */
    handler->stats.events_processed++;
    
    return true;
}

/* Cleanup and destroy handler */
void input_handler_destroy(InputHandler* handler) {
    if (!handler) {
        return;
    }
    
    /* Stop if running */
    if (handler->running) {
        input_handler_stop(handler);
    }
    
    /* Cleanup source */
    if (handler->source) {
        handler->source->cleanup(handler->source);
        free(handler->source);
    }
    
    /* Log final statistics */
    log_info("Input handler statistics:");
    log_info("  Total events: %llu", handler->stats.events_processed);
    log_info("  Touch events: %llu", handler->stats.touch_events);
    log_info("  Mouse events: %llu", handler->stats.mouse_events);
    log_info("  Keyboard events: %llu", handler->stats.keyboard_events);
    
    free(handler);
}