#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "widget.h"

// Widget creation function type
typedef Widget* (*widget_creator_func)(const char* id, void* params);

// Widget factory - creates widgets by type name
typedef struct WidgetFactory {
    // Registered widget types
    struct {
        char type_name[64];
        widget_creator_func creator;
    } *creators;
    size_t creator_count;
    size_t creator_capacity;
} WidgetFactory;

// Factory lifecycle
WidgetFactory* widget_factory_create(void);
void widget_factory_destroy(WidgetFactory* factory);

// Type registration
bool widget_factory_register(WidgetFactory* factory,
                           const char* type_name,
                           widget_creator_func creator);
bool widget_factory_unregister(WidgetFactory* factory, const char* type_name);

// Widget creation
Widget* widget_factory_create_widget(WidgetFactory* factory,
                                   const char* type_name,
                                   const char* id,
                                   void* params);

// Built-in widget creators
Widget* widget_factory_create_button(const char* id, void* params);
Widget* widget_factory_create_weather(const char* id, void* params);
Widget* widget_factory_create_label(const char* id, void* params);
Widget* widget_factory_create_container(const char* id, void* params);

// Default factory with built-in widgets registered
WidgetFactory* widget_factory_create_default(void);

// Parameters for built-in widgets
typedef struct {
    const char* label;
} ButtonParams;

typedef struct {
    const char* location;
} WeatherParams;

typedef struct {
    const char* text;
} LabelParams;

typedef struct {
    int columns;
    int rows;
} ContainerParams;

#endif // WIDGET_FACTORY_H