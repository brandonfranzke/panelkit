#ifndef BUTTON_WIDGET_H
#define BUTTON_WIDGET_H

#include "../widget.h"
#include "../../events/event_types.h"

// Forward declaration
typedef struct ButtonWidget ButtonWidget;

// Button click callback
typedef void (*button_click_callback)(ButtonWidget* button, void* user_data);

// Button widget - interactive container that handles click events
typedef struct ButtonWidget {
    Widget base;  // Must be first member for casting
    
    // Appearance (background colors for different states)
    SDL_Color normal_color;
    SDL_Color hover_color;
    SDL_Color pressed_color;
    SDL_Color disabled_color;
    
    // Callback
    button_click_callback on_click;
    void* user_data;              // [OWNER: caller] [TYPE: app-specific]
    
    // Optional event to publish on click
    char* publish_event;          // [OWNER: widget] [ALLOC: strdup]
    ButtonEventData* publish_data; // [OWNER: widget] [ALLOC: malloc]
    size_t publish_data_size;
} ButtonWidget;

/**
 * Create a new button widget.
 * 
 * @param id Unique identifier for the button
 * @return New button widget or NULL on error (caller owns)
 * @note Add text using widget_add_child with a text widget
 */
ButtonWidget* button_widget_create(const char* id);

/**
 * Set button colors for different states.
 * 
 * @param button Button widget to configure
 * @param normal Color when button is in normal state
 * @param hover Color when mouse is over button
 * @param pressed Color when button is being pressed
 * @param disabled Color when button is disabled
 */
void button_widget_set_colors(ButtonWidget* button,
                            SDL_Color normal,
                            SDL_Color hover,
                            SDL_Color pressed,
                            SDL_Color disabled);

/**
 * Set callback function for button clicks.
 * 
 * @param button Button widget to configure
 * @param callback Function to call on click (can be NULL)
 * @param user_data User data passed to callback (not owned)
 */
void button_widget_set_click_callback(ButtonWidget* button,
                                    button_click_callback callback,
                                    void* user_data);

/**
 * Configure event to publish when button is clicked.
 * 
 * @param button Button widget to configure
 * @param event_name Event name to publish (copied)
 * @param data Event data to publish (copied)
 * @param data_size Size of event data
 * @note Set event_name to NULL to disable event publishing
 */
void button_widget_set_publish_event(ButtonWidget* button,
                                   const char* event_name,
                                   void* data,
                                   size_t data_size);

/**
 * Programmatically trigger a button click.
 * 
 * @param button Button to click
 * @note Calls callback and publishes event if configured
 */
void button_widget_click(ButtonWidget* button);

#endif // BUTTON_WIDGET_H