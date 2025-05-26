#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include "../page_widget.h"
#include "../widget_factory.h"

// Forward declarations
typedef struct EventSystem EventSystem;
typedef struct StateStore StateStore;
typedef struct ConfigManager ConfigManager;

// Settings page - configuration options
typedef struct SettingsPage {
    PageWidget base;  // Must be first member for casting
    
    // Page-specific widgets
    Widget* content_container;
    Widget* brightness_label;
    Widget* brightness_minus;
    Widget* brightness_plus;
    Widget* brightness_value;
    
    Widget* location_label;
    Widget* location_button;
    
    Widget* units_label;
    Widget* units_toggle;
    
    Widget* back_button;
    
    // Configuration
    ConfigManager* config_manager;
    
    // Current values
    int brightness;
    bool use_celsius;
} SettingsPage;

// Constructor
SettingsPage* settings_page_create(WidgetFactory* factory,
                                 EventSystem* event_system,
                                 StateStore* state_store,
                                 ConfigManager* config_manager);

// Update UI from current config
void settings_page_update_from_config(SettingsPage* page);

// Save current settings to config
void settings_page_save_to_config(SettingsPage* page);

#endif // SETTINGS_PAGE_H