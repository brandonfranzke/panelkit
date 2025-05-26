#include "home_page.h"
#include "../widgets/weather_widget.h"
#include "../widgets/button_widget.h"
#include "../../events/event_system.h"
#include "../../state/state_store.h"
#include "../../config/config_manager.h"
#include <stdlib.h>
#include <stdio.h>

// Simple logging macros
#ifndef log_info
#define log_info(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

// Button callbacks
static void on_refresh_clicked(ButtonWidget* button, void* user_data) {
    HomePage* page = (HomePage*)user_data;
    log_info("Refresh button clicked on home page");
    home_page_refresh_data(page);
}

static void on_settings_clicked(ButtonWidget* button, void* user_data) {
    HomePage* page = (HomePage*)user_data;
    log_info("Settings button clicked");
    
    // Publish navigation event
    if (page->base.base.event_system) {
        struct { int target_page; } nav_data = { 1 };  // Settings is page 1
        event_publish(page->base.base.event_system, "navigation.goto", 
                     &nav_data, sizeof(nav_data));
    }
}

// Page lifecycle callbacks
static void home_page_on_enter(PageWidget* page) {
    HomePage* home = (HomePage*)page;
    log_info("Entered home page");
    
    // Update weather location from config
    home_page_update_weather_location(home);
    
    // Request fresh data
    home_page_refresh_data(home);
}

HomePage* home_page_create(WidgetFactory* factory,
                          EventSystem* event_system,
                          StateStore* state_store,
                          ConfigManager* config_manager) {
    if (!factory) {
        log_error("Widget factory required to create home page");
        return NULL;
    }
    
    HomePage* page = calloc(1, sizeof(HomePage));
    if (!page) {
        log_error("Failed to allocate home page");
        return NULL;
    }
    
    // Initialize base page
    PageWidget* base = &page->base;
    if (!page_widget_create("home_page", "Home")) {
        free(page);
        return NULL;
    }
    
    // Copy initialized data
    *base = *page_widget_create("home_page", "Home");
    
    // Connect systems
    widget_connect_systems(&base->base, event_system, state_store);
    
    // Store config reference
    page->config_manager = config_manager;
    
    // Set page callbacks
    page_widget_set_enter_callback(base, home_page_on_enter);
    
    // Create content container
    page->content_container = page_widget_create_content_container(base);
    if (!page->content_container) {
        widget_destroy(&base->base);
        return NULL;
    }
    
    // Create weather widget
    const char* location = "New York";  // Default, will be updated from config
    if (config_manager) {
        // Try to get location from config
        // ConfigValue* loc_value = config_manager_get(config_manager, "weather.location");
        // if (loc_value && loc_value->type == CONFIG_VALUE_STRING) {
        //     location = loc_value->string_value;
        // }
    }
    
    WeatherParams weather_params = {location};
    page->weather_widget = widget_factory_create_widget(factory, "weather", 
                                                      "home_weather", &weather_params);
    if (page->weather_widget) {
        widget_set_relative_bounds(page->weather_widget, 20, 20, 280, 180);
        widget_add_child(page->content_container, page->weather_widget);
    }
    
    // Create refresh button
    ButtonParams refresh_params = {"Refresh"};
    page->refresh_button = widget_factory_create_widget(factory, "button",
                                                      "refresh_btn", &refresh_params);
    if (page->refresh_button) {
        widget_set_relative_bounds(page->refresh_button, 20, 220, 120, 50);
        ButtonWidget* btn = (ButtonWidget*)page->refresh_button;
        button_widget_set_click_callback(btn, on_refresh_clicked, page);
        
        // Set button to publish refresh event
        button_widget_set_publish_event(btn, "weather.request", 
                                      &weather_params, sizeof(weather_params));
        
        widget_add_child(page->content_container, page->refresh_button);
    }
    
    // Create settings button
    ButtonParams settings_params = {"Settings"};
    page->settings_button = widget_factory_create_widget(factory, "button",
                                                       "settings_btn", &settings_params);
    if (page->settings_button) {
        widget_set_relative_bounds(page->settings_button, 160, 220, 120, 50);
        ButtonWidget* btn = (ButtonWidget*)page->settings_button;
        button_widget_set_click_callback(btn, on_settings_clicked, page);
        widget_add_child(page->content_container, page->settings_button);
    }
    
    // Add some info labels
    LabelParams info1_params = {"Temperature data updates every 5 minutes"};
    Widget* info1 = widget_factory_create_widget(factory, "label", 
                                               "info1", &info1_params);
    if (info1) {
        widget_set_relative_bounds(info1, 20, 290, 280, 30);
        widget_add_child(page->content_container, info1);
    }
    
    // Update content height for scrolling
    page_widget_update_content_height(base);
    
    log_info("Created home page with weather widget and buttons");
    return page;
}

void home_page_update_weather_location(HomePage* page) {
    if (!page || !page->weather_widget) {
        return;
    }
    
    // Get location from config
    const char* location = "New York";  // Default
    
    if (page->config_manager) {
        // ConfigValue* loc_value = config_manager_get(page->config_manager, "weather.location");
        // if (loc_value && loc_value->type == CONFIG_VALUE_STRING) {
        //     location = loc_value->string_value;
        // }
    }
    
    // Update weather widget
    WeatherWidget* weather = (WeatherWidget*)page->weather_widget;
    weather_widget_set_location(weather, location);
    
    log_debug("Updated weather location to '%s'", location);
}

void home_page_refresh_data(HomePage* page) {
    if (!page || !page->weather_widget) {
        return;
    }
    
    // Request weather update
    WeatherWidget* weather = (WeatherWidget*)page->weather_widget;
    weather_widget_request_update(weather);
    
    // Could also refresh other data sources here
    
    log_debug("Refreshing home page data");
}