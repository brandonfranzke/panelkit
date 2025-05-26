#ifndef EVENT_BUTTON_POC_H
#define EVENT_BUTTON_POC_H

#include <SDL.h>
#include <stdbool.h>

// Forward declarations
typedef struct EventSystem EventSystem;
typedef struct StateStore StateStore;

// Proof of concept event-aware button
typedef struct {
    // Basic button properties
    SDL_Rect rect;
    SDL_Color color;
    SDL_Color hover_color;
    SDL_Color text_color;
    char label[128];
    char display_value[256];  // Current value to display
    
    // Event subscription info
    char subscribed_event[128];
    bool is_subscribed;
    
    // State
    bool is_hovered;
    bool is_pressed;
    bool has_data;
    
    // References (not owned)
    EventSystem* event_system;
    StateStore* state_store;
} EventButtonPOC;

// Lifecycle
EventButtonPOC* event_button_poc_create(int x, int y, int width, int height, 
                                       const char* label, const char* event_name);
void event_button_poc_destroy(EventButtonPOC* button);

// Event system integration
bool event_button_poc_subscribe(EventButtonPOC* button, EventSystem* events, StateStore* store);
void event_button_poc_unsubscribe(EventButtonPOC* button);

// UI operations
void event_button_poc_handle_event(EventButtonPOC* button, SDL_Event* event);
void event_button_poc_update(EventButtonPOC* button);
void event_button_poc_render(EventButtonPOC* button, SDL_Renderer* renderer);

// Test helpers
void event_button_poc_simulate_weather_event(EventSystem* events, float temperature, const char* location);

#endif // EVENT_BUTTON_POC_H