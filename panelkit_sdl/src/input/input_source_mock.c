/**
 * @file input_source_mock.c
 * @brief Mock input source implementation
 * 
 * Provides simulated input events for testing without hardware dependencies.
 * Supports programmatic event generation and automated test patterns.
 */

#include "input_handler.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

/* Mock pattern states */
typedef enum {
    PATTERN_NONE = 0,
    PATTERN_TAP,
    PATTERN_SWIPE,
    PATTERN_PINCH,
    PATTERN_CIRCLE
} PatternType;

/* Private implementation data */
typedef struct MockData {
    /* Parent handler for event delivery */
    InputHandler* handler;
    
    /* Pattern generation thread */
    pthread_t pattern_thread;
    bool thread_running;
    
    /* Pattern configuration */
    PatternType pattern;
    int pattern_delay_ms;
    int pattern_duration_ms;
    
    /* Event queue for programmatic events */
    SDL_Event* event_queue;
    int queue_size;
    int queue_capacity;
    pthread_mutex_t queue_mutex;
} MockData;

/* Pattern generation thread */
static void* pattern_thread_func(void* arg) {
    InputSource* source = (InputSource*)arg;
    MockData* data = source->impl.mock;
    Uint32 start_time = SDL_GetTicks();
    
    while (data->thread_running) {
        Uint32 now = SDL_GetTicks();
        float t = (float)(now - start_time) / data->pattern_duration_ms;
        
        switch (data->pattern) {
            case PATTERN_TAP: {
                /* Single tap at center */
                if (t < 0.1f) {
                    SDL_Event event;
                    event.type = SDL_FINGERDOWN;
                    event.tfinger.touchId = 0;
                    event.tfinger.fingerId = 1;
                    event.tfinger.x = 0.5f;
                    event.tfinger.y = 0.5f;
                    event.tfinger.pressure = 1.0f;
                    input_handler_push_event(data->handler, &event);
                } else if (t >= 0.1f && t < 0.2f) {
                    SDL_Event event;
                    event.type = SDL_FINGERUP;
                    event.tfinger.touchId = 0;
                    event.tfinger.fingerId = 1;
                    event.tfinger.x = 0.5f;
                    event.tfinger.y = 0.5f;
                    event.tfinger.pressure = 0.0f;
                    input_handler_push_event(data->handler, &event);
                }
                break;
            }
            
            case PATTERN_SWIPE: {
                /* Horizontal swipe */
                if (t <= 1.0f) {
                    SDL_Event event;
                    if (t < 0.05f) {
                        event.type = SDL_FINGERDOWN;
                    } else {
                        event.type = SDL_FINGERMOTION;
                    }
                    event.tfinger.touchId = 0;
                    event.tfinger.fingerId = 1;
                    event.tfinger.x = 0.1f + t * 0.8f;
                    event.tfinger.y = 0.5f;
                    event.tfinger.pressure = 1.0f;
                    input_handler_push_event(data->handler, &event);
                } else if (t > 1.0f && t < 1.1f) {
                    SDL_Event event;
                    event.type = SDL_FINGERUP;
                    event.tfinger.touchId = 0;
                    event.tfinger.fingerId = 1;
                    event.tfinger.x = 0.9f;
                    event.tfinger.y = 0.5f;
                    event.tfinger.pressure = 0.0f;
                    input_handler_push_event(data->handler, &event);
                }
                break;
            }
            
            case PATTERN_CIRCLE: {
                /* Circular motion */
                if (t <= 1.0f) {
                    float angle = t * 2 * M_PI;
                    SDL_Event event;
                    if (t < 0.05f) {
                        event.type = SDL_FINGERDOWN;
                    } else {
                        event.type = SDL_FINGERMOTION;
                    }
                    event.tfinger.touchId = 0;
                    event.tfinger.fingerId = 1;
                    event.tfinger.x = 0.5f + 0.3f * cosf(angle);
                    event.tfinger.y = 0.5f + 0.3f * sinf(angle);
                    event.tfinger.pressure = 1.0f;
                    input_handler_push_event(data->handler, &event);
                } else if (t > 1.0f && t < 1.1f) {
                    SDL_Event event;
                    event.type = SDL_FINGERUP;
                    event.tfinger.touchId = 0;
                    event.tfinger.fingerId = 1;
                    event.tfinger.x = 0.8f;
                    event.tfinger.y = 0.5f;
                    event.tfinger.pressure = 0.0f;
                    input_handler_push_event(data->handler, &event);
                }
                break;
            }
            
            case PATTERN_PINCH: {
                /* Two-finger pinch gesture */
                if (t <= 1.0f) {
                    float distance = 0.4f * (1.0f - t);  /* Pinch in */
                    
                    /* First finger */
                    SDL_Event event1;
                    if (t < 0.05f) {
                        event1.type = SDL_FINGERDOWN;
                    } else {
                        event1.type = SDL_FINGERMOTION;
                    }
                    event1.tfinger.touchId = 0;
                    event1.tfinger.fingerId = 1;
                    event1.tfinger.x = 0.5f - distance/2;
                    event1.tfinger.y = 0.5f;
                    event1.tfinger.pressure = 1.0f;
                    input_handler_push_event(data->handler, &event1);
                    
                    /* Second finger */
                    SDL_Event event2;
                    event2.type = event1.type;
                    event2.tfinger.touchId = 0;
                    event2.tfinger.fingerId = 2;
                    event2.tfinger.x = 0.5f + distance/2;
                    event2.tfinger.y = 0.5f;
                    event2.tfinger.pressure = 1.0f;
                    input_handler_push_event(data->handler, &event2);
                } else if (t > 1.0f && t < 1.1f) {
                    /* Release both fingers */
                    SDL_Event event1;
                    event1.type = SDL_FINGERUP;
                    event1.tfinger.touchId = 0;
                    event1.tfinger.fingerId = 1;
                    event1.tfinger.x = 0.5f;
                    event1.tfinger.y = 0.5f;
                    event1.tfinger.pressure = 0.0f;
                    input_handler_push_event(data->handler, &event1);
                    
                    SDL_Event event2;
                    event2.type = SDL_FINGERUP;
                    event2.tfinger.touchId = 0;
                    event2.tfinger.fingerId = 2;
                    event2.tfinger.x = 0.5f;
                    event2.tfinger.y = 0.5f;
                    event2.tfinger.pressure = 0.0f;
                    input_handler_push_event(data->handler, &event2);
                }
                break;
            }
            
            default:
                break;
        }
        
        /* Reset pattern if completed */
        if (t > 1.2f) {
            start_time = SDL_GetTicks();
        }
        
        SDL_Delay(data->pattern_delay_ms);
    }
    
    return NULL;
}

/* InputSource interface implementation */
static bool mock_initialize(InputSource* source, const InputConfig* config) {
    (void)config; /* Unused for mock source */
    MockData* data = calloc(1, sizeof(MockData));
    if (!data) {
        SDL_LogError(SDL_LOG_CATEGORY_INPUT, "Failed to allocate mock data");
        return false;
    }
    
    data->pattern = PATTERN_NONE;
    data->pattern_delay_ms = 16;  /* ~60 FPS */
    data->pattern_duration_ms = 2000;
    
    /* Initialize event queue */
    data->queue_capacity = 100;
    data->event_queue = malloc(sizeof(SDL_Event) * data->queue_capacity);
    if (!data->event_queue) {
        free(data);
        return false;
    }
    
    if (pthread_mutex_init(&data->queue_mutex, NULL) != 0) {
        free(data->event_queue);
        free(data);
        return false;
    }
    
    source->impl.mock = data;
    source->name = "Mock Input";
    
    SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Mock input source initialized");
    return true;
}

static bool mock_start(InputSource* source, InputHandler* handler) {
    MockData* data = source->impl.mock;
    data->handler = handler;
    data->thread_running = true;
    
    /* Note: Pattern thread is started on demand via input_mock_start_pattern */
    
    return true;
}

static void mock_stop(InputSource* source) {
    MockData* data = source->impl.mock;
    
    /* Stop pattern thread if running */
    if (data->thread_running) {
        data->thread_running = false;
        if (data->pattern != PATTERN_NONE) {
            pthread_join(data->pattern_thread, NULL);
        }
    }
}

static bool mock_get_capabilities(InputSource* source, InputCapabilities* caps) {
    (void)source; /* Unused */
    caps->has_touch = true;
    caps->has_mouse = false;
    caps->has_keyboard = false;
    caps->max_touch_points = 10;
    caps->touch_x_min = 0;
    caps->touch_x_max = 1;
    caps->touch_y_min = 0;
    caps->touch_y_max = 1;
    return true;
}

static void mock_cleanup(InputSource* source) {
    MockData* data = source->impl.mock;
    
    mock_stop(source);
    
    pthread_mutex_destroy(&data->queue_mutex);
    free(data->event_queue);
    free(data);
    
    SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Mock input source cleaned up");
}

/* Public API for mock control */
void input_mock_queue_event(InputSource* source, SDL_Event* event) {
    if (!source || source->type != INPUT_SOURCE_MOCK) return;
    
    MockData* data = source->impl.mock;
    
    pthread_mutex_lock(&data->queue_mutex);
    if (data->queue_size < data->queue_capacity) {
        data->event_queue[data->queue_size++] = *event;
    }
    pthread_mutex_unlock(&data->queue_mutex);
}

void input_mock_start_pattern(InputSource* source, int pattern) {
    if (!source || source->type != INPUT_SOURCE_MOCK) return;
    
    MockData* data = source->impl.mock;
    
    /* Stop existing pattern */
    if (data->pattern != PATTERN_NONE && data->thread_running) {
        data->thread_running = false;
        pthread_join(data->pattern_thread, NULL);
    }
    
    data->pattern = pattern;
    
    if (pattern != PATTERN_NONE) {
        data->thread_running = true;
        pthread_create(&data->pattern_thread, NULL, pattern_thread_func, source);
    }
}

void input_mock_stop_pattern(InputSource* source) {
    input_mock_start_pattern(source, PATTERN_NONE);
}

void input_mock_configure_pattern(InputSource* source, int delay_ms, int duration_ms) {
    if (!source || source->type != INPUT_SOURCE_MOCK) return;
    
    MockData* data = source->impl.mock;
    data->pattern_delay_ms = delay_ms;
    data->pattern_duration_ms = duration_ms;
}

/* Create mock input source */
InputSource* input_source_mock_create(void) {
    InputSource* source = calloc(1, sizeof(InputSource));
    if (!source) return NULL;
    
    source->type = INPUT_SOURCE_MOCK;
    source->initialize = mock_initialize;
    source->start = mock_start;
    source->stop = mock_stop;
    source->get_capabilities = mock_get_capabilities;
    source->cleanup = mock_cleanup;
    
    return source;
}