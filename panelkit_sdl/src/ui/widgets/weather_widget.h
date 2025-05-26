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

// Constructor
WeatherWidget* weather_widget_create(const char* id, const char* location);

// Configuration
void weather_widget_set_location(WeatherWidget* widget, const char* location);
void weather_widget_set_units(WeatherWidget* widget, bool use_celsius);
void weather_widget_set_display_options(WeatherWidget* widget, 
                                      bool show_humidity, 
                                      bool show_description);

// Manual data update (for testing)
void weather_widget_set_data(WeatherWidget* widget, const WeatherData* data);

// Request weather update
void weather_widget_request_update(WeatherWidget* widget);

#endif // WEATHER_WIDGET_H