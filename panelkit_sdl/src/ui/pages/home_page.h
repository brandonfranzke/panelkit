#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include "../page_widget.h"
#include "../widget_factory.h"

// Forward declarations
typedef struct EventSystem EventSystem;
typedef struct StateStore StateStore;
typedef struct ConfigManager ConfigManager;

// Home page - main dashboard with weather and buttons
typedef struct HomePage {
    PageWidget base;  // Must be first member for casting
    
    // Page-specific widgets
    Widget* weather_widget;
    Widget* refresh_button;
    Widget* settings_button;
    Widget* content_container;
    
    // Configuration
    ConfigManager* config_manager;
} HomePage;

// Constructor
HomePage* home_page_create(WidgetFactory* factory,
                          EventSystem* event_system,
                          StateStore* state_store,
                          ConfigManager* config_manager);

// Update weather location from config
void home_page_update_weather_location(HomePage* page);

// Refresh data
void home_page_refresh_data(HomePage* page);

#endif // HOME_PAGE_H