#ifndef WEATHER_WIDGET_H
#define WEATHER_WIDGET_H

#include "../widget.h"
#include <time.h>

// Weather data structure
typedef struct {
    float temperature;
    float humidity;
    char location[64];
    char description[128];
    time_t timestamp;
} WeatherData;

// Weather widget - displays weather information and subscribes to weather events
typedef struct WeatherWidget {
    Widget base;  // Must be first member for casting
    
    // Current weather data
    WeatherData current_weather;
    bool has_data;
    
    // Display configuration
    bool show_humidity;
    bool show_description;
    bool use_celsius;
    
    // Update tracking
    time_t last_update;
    int update_interval;  // seconds
} WeatherWidget;

/**
 * Create a new weather display widget.
 * 
 * @param id Unique identifier for the widget
 * @param location Initial location for weather data
 * @return New weather widget or NULL on error (caller owns)
 * @note Widget subscribes to weather update events automatically
 */
WeatherWidget* weather_widget_create(const char* id, const char* location);

/**
 * Set the location for weather data.
 * 
 * @param widget Weather widget to update
 * @param location Location string (e.g., "San Francisco, CA")
 * @note Triggers automatic weather update request
 */
void weather_widget_set_location(WeatherWidget* widget, const char* location);

/**
 * Set temperature display units.
 * 
 * @param widget Weather widget to update
 * @param use_celsius True for Celsius, false for Fahrenheit
 */
void weather_widget_set_units(WeatherWidget* widget, bool use_celsius);

/**
 * Configure which weather information to display.
 * 
 * @param widget Weather widget to update
 * @param show_humidity True to show humidity percentage
 * @param show_description True to show weather description text
 */
void weather_widget_set_display_options(WeatherWidget* widget, 
                                      bool show_humidity, 
                                      bool show_description);

/**
 * Manually set weather data (primarily for testing).
 * 
 * @param widget Weather widget to update
 * @param data Weather data to display (copied)
 */
void weather_widget_set_data(WeatherWidget* widget, const WeatherData* data);

/**
 * Request a weather update from the API.
 * 
 * @param widget Weather widget requesting update
 * @note Publishes weather.request event if connected to event system
 */
void weather_widget_request_update(WeatherWidget* widget);

#endif // WEATHER_WIDGET_H