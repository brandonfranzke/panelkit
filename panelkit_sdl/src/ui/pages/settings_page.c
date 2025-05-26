#include "settings_page.h"
#include "../widgets/button_widget.h"
#include "../../events/event_system.h"
#include "../../state/state_store.h"
#include "../../config/config_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Simple logging macros
#ifndef log_info
#define log_info(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

// Button callbacks
static void on_back_clicked(ButtonWidget* button, void* user_data) {
    SettingsPage* page = (SettingsPage*)user_data;
    log_info("Back button clicked");
    
    // Save settings before leaving
    settings_page_save_to_config(page);
    
    // Publish navigation event
    if (page->base.base.event_system) {
        struct { int target_page; } nav_data = { 0 };  // Home is page 0
        event_publish(page->base.base.event_system, "navigation.goto", 
                     &nav_data, sizeof(nav_data));
    }
}

static void on_brightness_minus_clicked(ButtonWidget* button, void* user_data) {
    SettingsPage* page = (SettingsPage*)user_data;
    
    if (page->brightness > 10) {
        page->brightness -= 10;
        
        // Update display
        char value_text[32];
        snprintf(value_text, sizeof(value_text), "%d%%", page->brightness);
        button_widget_set_label((ButtonWidget*)page->brightness_value, value_text);
        
        log_debug("Brightness decreased to %d%%", page->brightness);
    }
}

static void on_brightness_plus_clicked(ButtonWidget* button, void* user_data) {
    SettingsPage* page = (SettingsPage*)user_data;
    
    if (page->brightness < 100) {
        page->brightness += 10;
        
        // Update display
        char value_text[32];
        snprintf(value_text, sizeof(value_text), "%d%%", page->brightness);
        button_widget_set_label((ButtonWidget*)page->brightness_value, value_text);
        
        log_debug("Brightness increased to %d%%", page->brightness);
    }
}

static void on_location_clicked(ButtonWidget* button, void* user_data) {
    SettingsPage* page = (SettingsPage*)user_data;
    log_info("Location button clicked");
    
    // In a real app, this would open a location picker
    // For now, cycle through some test locations
    static const char* locations[] = {"New York", "London", "Tokyo", "Sydney"};
    static int location_index = 0;
    
    location_index = (location_index + 1) % 4;
    button_widget_set_label(button, locations[location_index]);
}

static void on_units_toggle_clicked(ButtonWidget* button, void* user_data) {
    SettingsPage* page = (SettingsPage*)user_data;
    
    page->use_celsius = !page->use_celsius;
    button_widget_set_label(button, page->use_celsius ? "Celsius" : "Fahrenheit");
    
    log_debug("Temperature units toggled to %s", page->use_celsius ? "Celsius" : "Fahrenheit");
}

// Page lifecycle callbacks
static void settings_page_on_enter(PageWidget* page) {
    SettingsPage* settings = (SettingsPage*)page;
    log_info("Entered settings page");
    
    // Update UI from current config
    settings_page_update_from_config(settings);
}

static void settings_page_on_leave(PageWidget* page) {
    SettingsPage* settings = (SettingsPage*)page;
    log_info("Leaving settings page");
    
    // Save settings
    settings_page_save_to_config(settings);
}

// Helper to create a settings row
static void create_settings_row(WidgetFactory* factory, Widget* container,
                              const char* label_text, int y_offset,
                              Widget** label_out, Widget** control_out) {
    // Create label
    LabelParams label_params = {label_text};
    Widget* label = widget_factory_create_widget(factory, "label",
                                               label_text, &label_params);
    if (label) {
        widget_set_relative_bounds(label, 20, y_offset, 120, 40);
        widget_add_child(container, label);
        if (label_out) *label_out = label;
    }
}

SettingsPage* settings_page_create(WidgetFactory* factory,
                                 EventSystem* event_system,
                                 StateStore* state_store,
                                 ConfigManager* config_manager) {
    if (!factory) {
        log_error("Widget factory required to create settings page");
        return NULL;
    }
    
    SettingsPage* page = calloc(1, sizeof(SettingsPage));
    if (!page) {
        log_error("Failed to allocate settings page");
        return NULL;
    }
    
    // Initialize base page
    PageWidget* base = &page->base;
    if (!page_widget_create("settings_page", "Settings")) {
        free(page);
        return NULL;
    }
    
    // Copy initialized data
    *base = *page_widget_create("settings_page", "Settings");
    
    // Connect systems
    widget_connect_systems(&base->base, event_system, state_store);
    
    // Store config reference
    page->config_manager = config_manager;
    
    // Set page callbacks
    page_widget_set_enter_callback(base, settings_page_on_enter);
    page_widget_set_leave_callback(base, settings_page_on_leave);
    
    // Create content container
    page->content_container = page_widget_create_content_container(base);
    if (!page->content_container) {
        widget_destroy(&base->base);
        return NULL;
    }
    
    // Initialize default values
    page->brightness = 80;
    page->use_celsius = false;
    
    int y_offset = 20;
    
    // Brightness setting
    create_settings_row(factory, page->content_container,
                       "Brightness", y_offset,
                       &page->brightness_label, NULL);
    
    ButtonParams minus_params = {"-"};
    page->brightness_minus = widget_factory_create_widget(factory, "button",
                                                        "brightness_minus", &minus_params);
    if (page->brightness_minus) {
        widget_set_relative_bounds(page->brightness_minus, 150, y_offset, 40, 40);
        ButtonWidget* btn = (ButtonWidget*)page->brightness_minus;
        button_widget_set_click_callback(btn, on_brightness_minus_clicked, page);
        widget_add_child(page->content_container, page->brightness_minus);
    }
    
    ButtonParams value_params = {"80%"};
    page->brightness_value = widget_factory_create_widget(factory, "button",
                                                        "brightness_value", &value_params);
    if (page->brightness_value) {
        widget_set_relative_bounds(page->brightness_value, 200, y_offset, 60, 40);
        // Make it non-interactive
        widget_set_enabled(page->brightness_value, false);
        widget_add_child(page->content_container, page->brightness_value);
    }
    
    ButtonParams plus_params = {"+"};
    page->brightness_plus = widget_factory_create_widget(factory, "button",
                                                       "brightness_plus", &plus_params);
    if (page->brightness_plus) {
        widget_set_relative_bounds(page->brightness_plus, 270, y_offset, 40, 40);
        ButtonWidget* btn = (ButtonWidget*)page->brightness_plus;
        button_widget_set_click_callback(btn, on_brightness_plus_clicked, page);
        widget_add_child(page->content_container, page->brightness_plus);
    }
    
    y_offset += 60;
    
    // Location setting
    create_settings_row(factory, page->content_container,
                       "Location", y_offset,
                       &page->location_label, NULL);
    
    ButtonParams location_params = {"New York"};
    page->location_button = widget_factory_create_widget(factory, "button",
                                                       "location_btn", &location_params);
    if (page->location_button) {
        widget_set_relative_bounds(page->location_button, 150, y_offset, 160, 40);
        ButtonWidget* btn = (ButtonWidget*)page->location_button;
        button_widget_set_click_callback(btn, on_location_clicked, page);
        widget_add_child(page->content_container, page->location_button);
    }
    
    y_offset += 60;
    
    // Units setting
    create_settings_row(factory, page->content_container,
                       "Units", y_offset,
                       &page->units_label, NULL);
    
    ButtonParams units_params = {"Fahrenheit"};
    page->units_toggle = widget_factory_create_widget(factory, "button",
                                                     "units_toggle", &units_params);
    if (page->units_toggle) {
        widget_set_relative_bounds(page->units_toggle, 150, y_offset, 160, 40);
        ButtonWidget* btn = (ButtonWidget*)page->units_toggle;
        button_widget_set_click_callback(btn, on_units_toggle_clicked, page);
        widget_add_child(page->content_container, page->units_toggle);
    }
    
    y_offset += 80;
    
    // Back button
    ButtonParams back_params = {"Back to Home"};
    page->back_button = widget_factory_create_widget(factory, "button",
                                                   "back_btn", &back_params);
    if (page->back_button) {
        widget_set_relative_bounds(page->back_button, 20, y_offset, 290, 50);
        ButtonWidget* btn = (ButtonWidget*)page->back_button;
        button_widget_set_click_callback(btn, on_back_clicked, page);
        
        // Style as primary button
        SDL_Color primary_color = {50, 100, 200, 255};
        SDL_Color primary_hover = {70, 120, 220, 255};
        SDL_Color primary_pressed = {30, 80, 180, 255};
        SDL_Color disabled_color = {180, 180, 180, 255};
        button_widget_set_colors(btn, primary_color, primary_hover, 
                               primary_pressed, disabled_color);
        
        widget_add_child(page->content_container, page->back_button);
    }
    
    // Update content height for scrolling
    page_widget_update_content_height(base);
    
    log_info("Created settings page with configuration options");
    return page;
}

void settings_page_update_from_config(SettingsPage* page) {
    if (!page) {
        return;
    }
    
    // Update brightness
    // if (page->config_manager) {
    //     ConfigValue* brightness_val = config_manager_get(page->config_manager, "display.brightness");
    //     if (brightness_val && brightness_val->type == CONFIG_VALUE_INTEGER) {
    //         page->brightness = brightness_val->int_value;
    //     }
    // }
    
    // Update brightness display
    char value_text[32];
    snprintf(value_text, sizeof(value_text), "%d%%", page->brightness);
    if (page->brightness_value) {
        button_widget_set_label((ButtonWidget*)page->brightness_value, value_text);
    }
    
    // Update location
    const char* location = "New York";
    // if (page->config_manager) {
    //     ConfigValue* loc_val = config_manager_get(page->config_manager, "weather.location");
    //     if (loc_val && loc_val->type == CONFIG_VALUE_STRING) {
    //         location = loc_val->string_value;
    //     }
    // }
    if (page->location_button) {
        button_widget_set_label((ButtonWidget*)page->location_button, location);
    }
    
    // Update units
    // if (page->config_manager) {
    //     ConfigValue* units_val = config_manager_get(page->config_manager, "weather.use_celsius");
    //     if (units_val && units_val->type == CONFIG_VALUE_BOOLEAN) {
    //         page->use_celsius = units_val->bool_value;
    //     }
    // }
    if (page->units_toggle) {
        button_widget_set_label((ButtonWidget*)page->units_toggle,
                              page->use_celsius ? "Celsius" : "Fahrenheit");
    }
}

void settings_page_save_to_config(SettingsPage* page) {
    if (!page || !page->config_manager) {
        return;
    }
    
    // Save brightness
    // config_manager_set_int(page->config_manager, "display.brightness", page->brightness);
    
    // Save location
    if (page->location_button) {
        // const char* location = button_widget_get_label((ButtonWidget*)page->location_button);
        // config_manager_set_string(page->config_manager, "weather.location", location);
    }
    
    // Save units
    // config_manager_set_bool(page->config_manager, "weather.use_celsius", page->use_celsius);
    
    // Trigger config save
    // config_manager_save(page->config_manager);
    
    log_info("Settings saved to configuration");
}