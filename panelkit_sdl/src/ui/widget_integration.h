#ifndef WIDGET_INTEGRATION_H
#define WIDGET_INTEGRATION_H

#include "../state/state_store.h"
#include "../events/event_system.h"
#include <stdint.h>

// Forward declarations
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Color SDL_Color;
typedef struct WidgetManager WidgetManager;
typedef struct WidgetFactory WidgetFactory;
typedef struct Widget Widget;

// Widget integration layer - runs parallel to existing system
// This layer gradually replaces internal state without changing visuals
typedef struct WidgetIntegration {
    // Widget system components (running in background)
    StateStore* state_store;
    EventSystem* event_system;
    
    // Widget components (shadow UI tree)
    WidgetManager* widget_manager;
    WidgetFactory* widget_factory;
    
    // Shadow widgets that mirror existing UI
    Widget* page_manager;  // Page manager widget that handles all pages
    Widget* page_widgets[2];  // Mirror the 2 pages
    Widget* button_widgets[2][9];  // Mirror buttons on each page (max 9 per page)
    int num_pages;
    
    // Renderer reference (for future widget rendering)
    SDL_Renderer* renderer;
    
    // Migration state tracking
    bool widget_system_enabled;
    bool events_enabled;
    bool state_tracking_enabled;
    bool shadow_widgets_created;
    
    // Screen dimensions
    int screen_width;
    int screen_height;
} WidgetIntegration;

// Lifecycle
WidgetIntegration* widget_integration_create(SDL_Renderer* renderer);
void widget_integration_destroy(WidgetIntegration* integration);

// Setup
void widget_integration_set_dimensions(WidgetIntegration* integration, int width, int height);

// Migration controls - enable components gradually
void widget_integration_enable_events(WidgetIntegration* integration);
void widget_integration_enable_state_tracking(WidgetIntegration* integration);
void widget_integration_enable_widgets(WidgetIntegration* integration);

// Event integration - mirror existing events into widget system
void widget_integration_mirror_touch_event(WidgetIntegration* integration, 
                                          int x, int y, bool is_down);
void widget_integration_mirror_button_press(WidgetIntegration* integration,
                                           int button_index, const char* button_text);
void widget_integration_mirror_page_change(WidgetIntegration* integration,
                                          int from_page, int to_page);

// State integration - mirror existing state into widget system
void widget_integration_mirror_user_data(WidgetIntegration* integration,
                                        const void* user_data, size_t data_size);
void widget_integration_mirror_api_state(WidgetIntegration* integration,
                                        const char* state_name, const char* value);

// Update - call from main loop
void widget_integration_update(WidgetIntegration* integration);

// Update widget rendering based on state
void widget_integration_update_rendering(WidgetIntegration* integration);

// Query functions - for gradual replacement of existing state
bool widget_integration_has_user_data(WidgetIntegration* integration);
void* widget_integration_get_user_data(WidgetIntegration* integration, size_t* size);

// Shadow widget creation - mirrors existing UI structure
void widget_integration_create_shadow_widgets(WidgetIntegration* integration);
void widget_integration_sync_button_state(WidgetIntegration* integration, 
                                        int page, int button_index, 
                                        const char* text, bool enabled);
void widget_integration_sync_page_state(WidgetIntegration* integration,
                                       int page_index, bool is_active);

// Get shadow widgets for inspection/testing
Widget* widget_integration_get_page_widget(WidgetIntegration* integration, int page);
Widget* widget_integration_get_button_widget(WidgetIntegration* integration, int page, int button);

// State management helpers
void widget_integration_init_app_state(WidgetIntegration* integration);
int widget_integration_get_current_page(WidgetIntegration* integration);
void widget_integration_update_fps(WidgetIntegration* integration, uint32_t fps);
bool widget_integration_get_show_debug(WidgetIntegration* integration);
bool widget_integration_get_quit(WidgetIntegration* integration);
void widget_integration_set_quit(WidgetIntegration* integration, bool quit);

// Button handling migration
void widget_integration_enable_button_handling(WidgetIntegration* integration);

// State synchronization (for gradual migration)
void widget_integration_sync_state_to_globals(WidgetIntegration* integration, 
                                             SDL_Color* bg_color, bool* show_time, bool* quit, int* page1_text_color);

#endif // WIDGET_INTEGRATION_H