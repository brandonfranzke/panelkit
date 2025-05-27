#include "widget_factory.h"
#include "widget.h"
#include "widgets/button_widget.h"
#include "widgets/weather_widget.h"
#include "widgets/page_manager_widget.h"
#include "widgets/text_widget.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"

#define INITIAL_CREATOR_CAPACITY 16

WidgetFactory* widget_factory_create(void) {
    WidgetFactory* factory = calloc(1, sizeof(WidgetFactory));
    if (!factory) {
        log_error("Failed to allocate widget factory");
        return NULL;
    }
    
    factory->creator_capacity = INITIAL_CREATOR_CAPACITY;
    factory->creators = calloc(factory->creator_capacity, 
                             sizeof(factory->creators[0]));
    if (!factory->creators) {
        free(factory);
        return NULL;
    }
    
    log_info("Created widget factory");
    return factory;
}

void widget_factory_destroy(WidgetFactory* factory) {
    if (!factory) {
        return;
    }
    
    free(factory->creators);
    free(factory);
    log_info("Destroyed widget factory");
}

bool widget_factory_register(WidgetFactory* factory,
                           const char* type_name,
                           widget_creator_func creator) {
    if (!factory || !type_name || !creator) {
        return false;
    }
    
    // Check if already registered
    for (size_t i = 0; i < factory->creator_count; i++) {
        if (strcmp(factory->creators[i].type_name, type_name) == 0) {
            log_error("Widget type '%s' already registered", type_name);
            return false;
        }
    }
    
    // Grow array if needed
    if (factory->creator_count >= factory->creator_capacity) {
        size_t new_capacity = factory->creator_capacity * 2;
        void* new_creators = realloc(factory->creators,
                                   new_capacity * sizeof(factory->creators[0]));
        if (!new_creators) {
            log_error("Failed to grow creators array");
            return false;
        }
        factory->creators = new_creators;
        factory->creator_capacity = new_capacity;
    }
    
    // Add creator
    strncpy(factory->creators[factory->creator_count].type_name,
            type_name, 
            sizeof(factory->creators[0].type_name) - 1);
    factory->creators[factory->creator_count].creator = creator;
    factory->creator_count++;
    
    log_info("Registered widget type '%s'", type_name);
    return true;
}

bool widget_factory_unregister(WidgetFactory* factory, const char* type_name) {
    if (!factory || !type_name) {
        return false;
    }
    
    for (size_t i = 0; i < factory->creator_count; i++) {
        if (strcmp(factory->creators[i].type_name, type_name) == 0) {
            // Shift remaining creators
            for (size_t j = i + 1; j < factory->creator_count; j++) {
                factory->creators[j - 1] = factory->creators[j];
            }
            factory->creator_count--;
            
            log_info("Unregistered widget type '%s'", type_name);
            return true;
        }
    }
    
    return false;
}

Widget* widget_factory_create_widget(WidgetFactory* factory,
                                   const char* type_name,
                                   const char* id,
                                   void* params) {
    if (!factory || !type_name || !id) {
        return NULL;
    }
    
    // Find creator
    for (size_t i = 0; i < factory->creator_count; i++) {
        if (strcmp(factory->creators[i].type_name, type_name) == 0) {
            Widget* widget = factory->creators[i].creator(id, params);
            if (widget) {
                log_debug("Created widget '%s' of type '%s'", id, type_name);
            }
            return widget;
        }
    }
    
    log_error("Unknown widget type '%s'", type_name);
    return NULL;
}

// Built-in widget creators

Widget* widget_factory_create_button(const char* id, void* params) {
    // Create empty button - label will be added as child widget
    ButtonWidget* button = button_widget_create(id);
    return button ? &button->base : NULL;
}

Widget* widget_factory_create_weather(const char* id, void* params) {
    WeatherParams* weather_params = (WeatherParams*)params;
    const char* location = weather_params ? weather_params->location : "Unknown";
    
    WeatherWidget* weather = weather_widget_create(id, location);
    return weather ? &weather->base : NULL;
}

Widget* widget_factory_create_page_manager(const char* id, void* params) {
    int page_count = params ? *(int*)params : 2;  // Default to 2 pages
    return page_manager_widget_create(id, page_count);
}

Widget* widget_factory_create_label(const char* id, void* params) {
    // For now, create a basic widget as a label
    Widget* label = widget_create(id, WIDGET_TYPE_LABEL);
    if (!label) {
        return NULL;
    }
    
    LabelParams* label_params = (LabelParams*)params;
    if (label_params && label_params->text) {
        // In a real implementation, we'd store the text
        // For now, just adjust the size based on text length
        label->bounds.w = (int)(strlen(label_params->text) * 8 + 20);
        label->bounds.h = 30;
    }
    
    // Labels are typically non-interactive
    label->background_color = (SDL_Color){255, 255, 255, 0};  // Transparent
    label->border_width = 0;
    
    return label;
}

Widget* widget_factory_create_container(const char* id, void* params) {
    Widget* container = widget_create(id, WIDGET_TYPE_CONTAINER);
    if (!container) {
        return NULL;
    }
    
    ContainerParams* container_params = (ContainerParams*)params;
    if (container_params) {
        // Store layout hints in bounds for now
        container->bounds.w = container_params->columns * 100;
        container->bounds.h = container_params->rows * 100;
    }
    
    // Containers typically have minimal styling
    container->background_color = (SDL_Color){240, 240, 240, 255};
    container->border_width = 0;
    container->padding = 10;
    
    return container;
}

WidgetFactory* widget_factory_create_default(void) {
    WidgetFactory* factory = widget_factory_create();
    if (!factory) {
        return NULL;
    }
    
    // Register built-in widgets
    widget_factory_register(factory, "button", widget_factory_create_button);
    widget_factory_register(factory, "weather", widget_factory_create_weather);
    widget_factory_register(factory, "label", widget_factory_create_label);
    widget_factory_register(factory, "container", widget_factory_create_container);
    
    log_info("Created default widget factory with built-in types");
    return factory;
}

// Type-safe widget creators (Phase 2)
struct ButtonWidget* widget_factory_create_button_typed(const char* id, const ButtonParams* params) {
    // Use the generic creator but cast the result safely
    Widget* widget = widget_factory_create_button(id, (void*)params);
    return CAST_TO_BUTTON(widget);
}

struct WeatherWidget* widget_factory_create_weather_typed(const char* id, const WeatherParams* params) {
    Widget* widget = widget_factory_create_weather(id, (void*)params);
    // Note: CAST_TO_WEATHER returns void*, need proper cast
    return (widget && widget->type == WIDGET_TYPE_WEATHER) ? (struct WeatherWidget*)widget : NULL;
}

struct TextWidget* widget_factory_create_label_typed(const char* id, const LabelParams* params) {
    Widget* widget = widget_factory_create_label(id, (void*)params);
    // Note: CAST_TO_LABEL returns void*, need proper cast
    return (widget && widget->type == WIDGET_TYPE_LABEL) ? (struct TextWidget*)widget : NULL;
}

Widget* widget_factory_create_container_typed(const char* id, const ContainerParams* params) {
    Widget* widget = widget_factory_create_container(id, (void*)params);
    return CAST_TO_CONTAINER(widget);
}