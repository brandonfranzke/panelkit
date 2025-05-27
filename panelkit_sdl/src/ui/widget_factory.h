#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "widget.h"

/**
 * Widget creation function prototype.
 * 
 * @param id Widget identifier
 * @param params Type-specific parameters (can be NULL)
 * @return New widget or NULL on error
 */
typedef Widget* (*widget_creator_func)(const char* id, void* params);

/**
 * Widget factory for dynamic widget creation by type name.
 * Maintains a registry of widget types and their creation functions.
 */
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

/**
 * Create a new widget factory.
 * 
 * @return New factory or NULL on error (caller owns)
 * @note Factory starts empty - register types before use
 */
WidgetFactory* widget_factory_create(void);

/**
 * Destroy a widget factory.
 * 
 * @param factory Factory to destroy (can be NULL)
 * @note Does not destroy created widgets - only the factory itself
 */
void widget_factory_destroy(WidgetFactory* factory);

// Type registration

/**
 * Register a widget type with the factory.
 * 
 * @param factory Target factory (required)
 * @param type_name Type identifier (required, max 63 chars)
 * @param creator Creation function (required)
 * @return true on success, false on error
 * @note Overwrites existing registration for same type_name
 */
bool widget_factory_register(WidgetFactory* factory,
                           const char* type_name,
                           widget_creator_func creator);

/**
 * Unregister a widget type from the factory.
 * 
 * @param factory Target factory (required)
 * @param type_name Type identifier to remove (required)
 * @return true if found and removed, false otherwise
 */
bool widget_factory_unregister(WidgetFactory* factory, const char* type_name);

// Parameters for built-in widgets

/** Button widget creation parameters */
typedef struct {
    const char* label;    /**< Button label text */
} ButtonParams;

/** Weather widget creation parameters */
typedef struct {
    const char* location; /**< Location for weather data */
} WeatherParams;

/** Label/text widget creation parameters */
typedef struct {
    const char* text;     /**< Display text */
} LabelParams;

/** Container widget creation parameters */
typedef struct {
    int columns;          /**< Number of columns */
    int rows;             /**< Number of rows */
} ContainerParams;

// Widget creation

/**
 * Create a widget using registered type.
 * 
 * @param factory Factory with registered types (required)
 * @param type_name Registered type name (required)
 * @param id Widget identifier (required)
 * @param params Type-specific parameters (can be NULL)
 * @return New widget or NULL on error (caller owns)
 * @note Type must be registered with widget_factory_register()
 */
Widget* widget_factory_create_widget(WidgetFactory* factory,
                                   const char* type_name,
                                   const char* id,
                                   void* params);

// Built-in widget creators

/**
 * Create a button widget.
 * 
 * @param id Widget identifier (required)
 * @param params ButtonParams or NULL for default
 * @return New button widget or NULL on error (caller owns)
 */
Widget* widget_factory_create_button(const char* id, void* params);

/**
 * Create a weather widget.
 * 
 * @param id Widget identifier (required)
 * @param params WeatherParams or NULL for default
 * @return New weather widget or NULL on error (caller owns)
 */
Widget* widget_factory_create_weather(const char* id, void* params);

/**
 * Create a label/text widget.
 * 
 * @param id Widget identifier (required)
 * @param params LabelParams or NULL for default
 * @return New label widget or NULL on error (caller owns)
 */
Widget* widget_factory_create_label(const char* id, void* params);

/**
 * Create a container widget.
 * 
 * @param id Widget identifier (required)
 * @param params ContainerParams or NULL for default
 * @return New container widget or NULL on error (caller owns)
 */
Widget* widget_factory_create_container(const char* id, void* params);

// Type-safe widget creators (Phase 2)

/**
 * Create a button widget with type safety.
 * 
 * @param id Widget identifier (required)
 * @param params Button parameters (can be NULL)
 * @return New button widget or NULL on error (caller owns)
 * @note Returns concrete ButtonWidget* type
 */
struct ButtonWidget* widget_factory_create_button_typed(const char* id, const ButtonParams* params);

/**
 * Create a weather widget with type safety.
 * 
 * @param id Widget identifier (required)
 * @param params Weather parameters (can be NULL)
 * @return New weather widget or NULL on error (caller owns)
 * @note Returns concrete WeatherWidget* type
 */
struct WeatherWidget* widget_factory_create_weather_typed(const char* id, const WeatherParams* params);

/**
 * Create a label widget with type safety.
 * 
 * @param id Widget identifier (required)
 * @param params Label parameters (can be NULL)
 * @return New text widget or NULL on error (caller owns)
 * @note Returns concrete TextWidget* type
 */
struct TextWidget* widget_factory_create_label_typed(const char* id, const LabelParams* params);

/**
 * Create a container widget with type safety.
 * 
 * @param id Widget identifier (required)
 * @param params Container parameters (can be NULL)
 * @return New container widget or NULL on error (caller owns)
 */
Widget* widget_factory_create_container_typed(const char* id, const ContainerParams* params);

// Default factory

/**
 * Create a factory with built-in widgets pre-registered.
 * 
 * @return New factory with standard widgets or NULL on error (caller owns)
 * @note Registers: button, weather, label, container types
 */
WidgetFactory* widget_factory_create_default(void);

#endif // WIDGET_FACTORY_H