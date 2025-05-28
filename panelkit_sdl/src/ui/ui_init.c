/**
 * @file ui_init.c
 * @brief Temporary UI initialization - will be replaced by layout/theme system
 * 
 * This file contains the hardcoded UI setup extracted from widget_integration.
 * It's intentionally minimal and will be completely replaced when the proper
 * layout and theme systems are implemented.
 */

#include "ui_init.h"
#include "widget_manager.h"
#include "widget_factory.h"
#include "page_widget.h"
#include "widgets/page_manager_widget.h"
#include "widgets/button_widget.h"
#include "widgets/text_widget.h"
#include "widgets/time_widget.h"
#include "widgets/weather_widget.h"
#include "widgets/data_display_widget.h"
#include "../events/event_system.h"
#include "../events/event_system_typed.h"
#include "../events/event_types.h"
#include "../state/state_store.h"
#include "../api/api_manager.h"
#include "core/logger.h"
#include "core/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global state that was in app.c - moving here temporarily
// These will be removed when layout/theme system is implemented
bool g_show_time = true;
static const char* button_labels[] = {
    "Blue", "Random", "Time", "Page 1", "Refresh", "Exit",
    "Button 7", "Button 8", "Button 9"
};

// Temporary storage for UI components
typedef struct {
    WidgetManager* widget_manager;
    WidgetFactory* widget_factory;
    Widget* page_manager;
    EventSystem* event_system;
    StateStore* state_store;
    int screen_width;
    int screen_height;
} UIContext;

static UIContext* g_ui_context = NULL;

// Button click handler - extracted from widget_integration_events.c
static void ui_button_click_handler(const ButtonEventData* button_data, void* context) {
    UIContext* ui = (UIContext*)context;
    if (!ui || !button_data) {
        return;
    }
    
    log_debug("Button click handler: page=%d button=%d text='%s'",
             button_data->page, button_data->button_index, button_data->button_text);
    
    // Handle actions based on page and button
    if (button_data->page == 0) { // Page 0 (first page)
        switch (button_data->button_index) {
            case 0: { // "Change Color" button - should transition to page 1
                int target_page = 1;
                event_publish_page_transition(ui->event_system, target_page);
                log_debug("Page transition to %d requested", target_page);
                break;
            }
        }
    }
    else if (button_data->page == 1) { // Page 1 (second page)
        switch (button_data->button_index) {
            case 0: { // Blue button
                SDL_Color blue_color = {41, 128, 185, 255};
                state_store_set(ui->state_store, "app", "bg_color", &blue_color, sizeof(SDL_Color));
                log_debug("Background color set to blue");
                break;
            }
            case 1: { // Random color button
                SDL_Color random_color = {
                    .r = rand() % 256,
                    .g = rand() % 256, 
                    .b = rand() % 256,
                    .a = 255
                };
                state_store_set(ui->state_store, "app", "bg_color", &random_color, sizeof(SDL_Color));
                log_debug("Background color set to random RGB(%d,%d,%d)",
                         random_color.r, random_color.g, random_color.b);
                break;
            }
            case 2: { // Time toggle button  
                g_show_time = !g_show_time;
                state_store_set(ui->state_store, "app", "show_time", &g_show_time, sizeof(bool));
                log_debug("Time display toggled to %s", g_show_time ? "enabled" : "disabled");
                
                // Update the button text
                Widget* button = widget_find_descendant(ui->page_manager, "page1_button2");
                if (button) {
                    Widget* text = widget_find_descendant(button, "page1_button2_text");
                    if (text) {
                        if (g_show_time) {
                            // Show current time
                            time_t now = time(NULL);
                            struct tm *tm_info = localtime(&now);
                            char time_text[64];
                            strftime(time_text, sizeof(time_text), "%H:%M:%S\n%Y-%b-%d", tm_info);
                            text_widget_set_text(text, time_text);
                        } else {
                            // Show "Time" label
                            text_widget_set_text(text, "Time");
                        }
                    }
                }
                break;
            }
            case 3: { // Go to Page 0 button
                int target_page = 0;
                event_publish_page_transition(ui->event_system, target_page);
                log_debug("Page transition to %d requested", target_page);
                break;
            }
            case 4: { // Refresh API data button
                uint32_t timestamp = SDL_GetTicks();
                event_publish_api_refresh_requested(ui->event_system, timestamp);
                log_debug("API refresh requested");
                break;
            }
            case 5: { // Exit button
                bool quit = true;
                state_store_set(ui->state_store, "app", "quit", &quit, sizeof(bool));
                log_debug("Application exit requested");
                break;
            }
        }
    }
}

// Page transition handler
static void ui_page_transition_handler(int target_page, void* context) {
    UIContext* ui = (UIContext*)context;
    if (!ui) return;
    
    log_debug("Page transition handler: transitioning to page %d", target_page);
    
    // Update state store with new page
    state_store_set(ui->state_store, "app", "current_page", &target_page, sizeof(int));
    
    // Use page manager widget
    if (ui->page_manager) {
        page_manager_transition_to(ui->page_manager, target_page);
    }
}

// API refresh handler
static void ui_api_refresh_handler(uint32_t timestamp, void* context) {
    UIContext* ui = (UIContext*)context;
    if (!ui || !ui->event_system) return;
    
    log_debug("API refresh handler: publishing system event");
    
    ApiRefreshData api_event;
    api_event.timestamp = SDL_GetTicks();
    strcpy(api_event.source, "ui_system");
    
    event_publish_api_refresh(ui->event_system, &api_event);
}

// Page changed callback
static void ui_page_changed_callback(int from_page, int to_page, void* user_data) {
    UIContext* ui = (UIContext*)user_data;
    if (!ui) return;
    
    log_debug("Page manager widget changed page: %d -> %d", from_page, to_page);
    
    // Update state store with new page
    state_store_set(ui->state_store, "app", "current_page", &to_page, sizeof(int));
}

/**
 * Initialize the UI system
 * This creates all widgets with hardcoded layout
 */
PkError ui_init(WidgetManager* widget_manager, 
                EventSystem* event_system,
                StateStore* state_store,
                int screen_width, int screen_height,
                TTF_Font* font_regular,
                TTF_Font* font_large,
                TTF_Font* font_small) {
    
    if (!widget_manager || !event_system || !state_store) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "ui_init: Required parameters are NULL");
        return PK_ERROR_NULL_PARAM;
    }
    
    // Create UI context
    g_ui_context = calloc(1, sizeof(UIContext));
    if (!g_ui_context) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "Failed to allocate UI context");
        return PK_ERROR_OUT_OF_MEMORY;
    }
    
    g_ui_context->widget_manager = widget_manager;
    g_ui_context->event_system = event_system;
    g_ui_context->state_store = state_store;
    g_ui_context->screen_width = screen_width;
    g_ui_context->screen_height = screen_height;
    
    // Store fonts in widget manager temporarily
    widget_manager_set_fonts(widget_manager, font_regular, font_large, font_small);
    
    // Create widget factory
    g_ui_context->widget_factory = widget_factory_create_default();
    if (!g_ui_context->widget_factory) {
        free(g_ui_context);
        g_ui_context = NULL;
        return pk_get_last_error();
    }
    
    
    // Initialize default app state
    int current_page = 0;
    state_store_set(state_store, "app", "current_page", &current_page, sizeof(int));
    state_store_set(state_store, "app", "show_time", &g_show_time, sizeof(bool));
    
    SDL_Color bg_color = {33, 33, 33, 255};
    state_store_set(state_store, "app", "bg_color", &bg_color, sizeof(SDL_Color));
    
    // Create page manager
    int page_count = 2;
    g_ui_context->page_manager = widget_factory_create_widget(
        g_ui_context->widget_factory, "page_manager", "page_manager", &page_count);
    
    if (!g_ui_context->page_manager) {
        widget_factory_destroy(g_ui_context->widget_factory);
        free(g_ui_context);
        g_ui_context = NULL;
        return pk_get_last_error();
    }
    
    // Set bounds and add to widget manager
    widget_set_bounds(g_ui_context->page_manager, 0, 0, screen_width, screen_height);
    widget_manager_add_root(widget_manager, g_ui_context->page_manager, "page_manager");
    widget_manager_set_active_root(widget_manager, "page_manager");
    
    // Set page change callback
    page_manager_set_page_changed_callback(g_ui_context->page_manager, 
                                          ui_page_changed_callback, g_ui_context);
    
    // Connect systems
    widget_connect_systems(g_ui_context->page_manager, event_system, state_store);
    
    // Create UI widgets for each page
    ui_create_page_widgets();
    
    // Subscribe to events
    event_subscribe_button_pressed(event_system, ui_button_click_handler, g_ui_context);
    event_subscribe_page_transition(event_system, ui_page_transition_handler, g_ui_context);
    event_subscribe_api_refresh_requested(event_system, ui_api_refresh_handler, g_ui_context);
    
    log_info("UI system initialized with %d pages", page_count);
    return PK_OK;
}

/**
 * Create widgets for all pages - extracted from widget_integration_widgets.c
 */
void ui_create_page_widgets(void) {
    if (!g_ui_context || !g_ui_context->page_manager) {
        return;
    }
    
    // Create pages first
    PageWidget* page0_widget = page_widget_create("page_0", "");
    PageWidget* page1_widget = page_widget_create("page_1", "");
    
    if (page0_widget && page1_widget) {
        // Add pages to page manager
        page_manager_add_page(g_ui_context->page_manager, 0, &page0_widget->base);
        page_manager_add_page(g_ui_context->page_manager, 1, &page1_widget->base);
    }
    
    // Page 0: Single button
    Widget* page0 = &page0_widget->base;
    if (page0) {
        // Add page title
        TTF_Font* large_font = NULL;
        widget_manager_get_fonts(g_ui_context->widget_manager, NULL, &large_font, NULL);
        
        Widget* title0 = text_widget_create("page0_title", "Main Menu", large_font);
        if (title0) {
            widget_set_bounds(title0, 0, 20, g_ui_context->screen_width, 40);
            text_widget_set_alignment(title0, TEXT_ALIGN_CENTER);
            widget_add_child(page0, title0);
        }
        
        // Create button
        ButtonWidget* btn = button_widget_create("page0_button0");
        if (btn) {
            Widget* button = &btn->base;
            widget_set_bounds(button, 20, 100, 200, 50);
            widget_add_child(page0, button);
            
            // Add text to button
            TTF_Font* regular_font = NULL;
            widget_manager_get_fonts(g_ui_context->widget_manager, &regular_font, NULL, NULL);
            
            Widget* text = text_widget_create("page0_button0_text", "Change Color", regular_font);
            if (text) {
                // Set text to fill button area
                widget_set_relative_bounds(text, 0, 0, 200, 50);
                widget_add_child(button, text);
            }
            
            // Connect button to event system
            widget_connect_systems(button, g_ui_context->event_system, g_ui_context->state_store);
            
            // Set up event publishing
            ButtonEventData* click_data = malloc(sizeof(ButtonEventData));
            if (click_data) {
                click_data->button_index = 0;
                click_data->page = 0;
                strncpy(click_data->button_text, "Change Color", 
                       sizeof(click_data->button_text) - 1);
                button_widget_set_publish_event(btn, "ui.button_pressed", 
                                              click_data, sizeof(*click_data));
                free(click_data); // button_widget copies the data
            }
        }
    }
    
    // Page 1: Grid of buttons
    Widget* page1 = &page1_widget->base;
    if (page1) {
        // Add page title
        TTF_Font* large_font = NULL;
        widget_manager_get_fonts(g_ui_context->widget_manager, NULL, &large_font, NULL);
        
        Widget* title1 = text_widget_create("page1_title", "Control Panel", large_font);
        if (title1) {
            widget_set_bounds(title1, 0, 10, g_ui_context->screen_width, 40);
            text_widget_set_alignment(title1, TEXT_ALIGN_CENTER);
            widget_add_child(page1, title1);
        }
        
        int button_width = (g_ui_context->screen_width - 40) / 3;
        int button_height = 60;
        
        for (int i = 0; i < 9; i++) {
            char button_id[32];
            snprintf(button_id, sizeof(button_id), "page1_button%d", i);
            
            ButtonWidget* btn = button_widget_create(button_id);
            if (btn) {
                Widget* button = &btn->base;
                
                // Calculate position
                int row = i / 3;
                int col = i % 3;
                int x = col * (button_width + 10) + 10;
                int y = row * (button_height + 10) + 50;
                
                widget_set_bounds(button, x, y, button_width, button_height);
                widget_add_child(page1, button);
                
                // Add text
                char text_id[32];
                snprintf(text_id, sizeof(text_id), "page1_button%d_text", i);
                
                TTF_Font* regular_font = NULL;
                widget_manager_get_fonts(g_ui_context->widget_manager, &regular_font, NULL, NULL);
                
                // Special handling for time button (button 2)
                if (i == 2) {
                    // Time button shows current time
                    time_t now = time(NULL);
                    struct tm *tm_info = localtime(&now);
                    char time_text[64];
                    strftime(time_text, sizeof(time_text), "%H:%M:%S\n%Y-%b-%d", tm_info);
                    
                    Widget* text = text_widget_create(text_id, time_text, regular_font);
                    if (text) {
                        widget_set_relative_bounds(text, 0, 0, button_width, button_height);
                        widget_add_child(button, text);
                    }
                } else {
                    Widget* text = text_widget_create(text_id, button_labels[i], regular_font);
                    if (text) {
                        // Set text to fill button area
                        widget_set_relative_bounds(text, 0, 0, button_width, button_height);
                        widget_add_child(button, text);
                    }
                }
                
                // Connect button to event system
                widget_connect_systems(button, g_ui_context->event_system, g_ui_context->state_store);
                
                // Set up event publishing
                ButtonEventData* click_data = malloc(sizeof(ButtonEventData));
                if (click_data) {
                    click_data->button_index = i;
                    click_data->page = 1;
                    strncpy(click_data->button_text, button_labels[i], 
                           sizeof(click_data->button_text) - 1);
                    button_widget_set_publish_event(btn, "ui.button_pressed", 
                                                  click_data, sizeof(*click_data));
                    free(click_data);
                }
            }
        }
        
        // No separate time widget - time is shown on button 2
        
        // Add weather widget
        WeatherWidget* weather = weather_widget_create("weather_display", "San Francisco");
        if (weather) {
            Widget* weather_widget = &weather->base;
            widget_set_bounds(weather_widget, 10, 
                            g_ui_context->screen_height - 110, 300, 100);
            widget_add_child(page1, weather_widget);
        }
        
        // Add data display widget
        TTF_Font* regular_font = NULL;
        TTF_Font* small_font = NULL;
        widget_manager_get_fonts(g_ui_context->widget_manager, &regular_font, NULL, &small_font);
        
        Widget* data_display = data_display_widget_create("user_data_display", 
                                                        regular_font, small_font);
        if (data_display) {
            widget_set_bounds(data_display, 
                            g_ui_context->screen_width - 320, 
                            g_ui_context->screen_height - 160, 300, 150);
            widget_add_child(page1, data_display);
            
            // Subscribe to user data updates
            widget_subscribe_event(data_display, "api.user_data_updated");
            
            // Set some placeholder data for demo
            data_display_widget_set_user_data(data_display, 
                                            "Demo User",
                                            "demo@panelkit.app", 
                                            "+1 (555) 123-4567",
                                            "San Francisco", 
                                            "CA");
        }
    }
}

/**
 * Update UI rendering based on current state
 */
void ui_update_rendering(void) {
    if (!g_ui_context) return;
    
    // Update time on button 2 if showing
    size_t size;
    time_t timestamp;
    bool* show_time = (bool*)state_store_get(g_ui_context->state_store, 
                                            "app", "show_time", &size, &timestamp);
    if (show_time) {
        Widget* button = widget_find_descendant(g_ui_context->page_manager, "page1_button2");
        if (button) {
            Widget* text = widget_find_descendant(button, "page1_button2_text");
            if (text) {
                if (*show_time) {
                    // Update time text
                    time_t now = time(NULL);
                    struct tm *tm_info = localtime(&now);
                    char time_text[64];
                    strftime(time_text, sizeof(time_text), "%H:%M:%S\n%Y-%b-%d", tm_info);
                    text_widget_set_text(text, time_text);
                } else {
                    // Show "Time" label
                    text_widget_set_text(text, "Time");
                }
            }
        }
        free(show_time);
    }
}

/**
 * Get the page manager widget
 */
Widget* ui_get_page_manager(void) {
    return g_ui_context ? g_ui_context->page_manager : NULL;
}

/**
 * Cleanup UI system
 */
void ui_cleanup(void) {
    if (!g_ui_context) return;
    
    // Widgets are owned by widget manager, so we don't destroy them here
    
    // Destroy factory
    if (g_ui_context->widget_factory) {
        widget_factory_destroy(g_ui_context->widget_factory);
    }
    
    free(g_ui_context);
    g_ui_context = NULL;
    
    log_info("UI system cleaned up");
}