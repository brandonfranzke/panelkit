#ifndef BUTTON_WIDGET_H
#define BUTTON_WIDGET_H

#include "../widget.h"

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
    void* user_data;
    
    // Optional event to publish on click
    char* publish_event;
    void* publish_data;
    size_t publish_data_size;
} ButtonWidget;

// Constructor - creates an empty button container
ButtonWidget* button_widget_create(const char* id);

// Configuration
void button_widget_set_colors(ButtonWidget* button,
                            SDL_Color normal,
                            SDL_Color hover,
                            SDL_Color pressed,
                            SDL_Color disabled);

// Click handling
void button_widget_set_click_callback(ButtonWidget* button,
                                    button_click_callback callback,
                                    void* user_data);

// Event publishing
void button_widget_set_publish_event(ButtonWidget* button,
                                   const char* event_name,
                                   void* data,
                                   size_t data_size);

// Programmatic click
void button_widget_click(ButtonWidget* button);

#endif // BUTTON_WIDGET_H