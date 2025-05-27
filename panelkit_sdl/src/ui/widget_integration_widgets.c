#include "widget_integration.h"
#include "widget_integration_internal.h"
#include "../state/state_store.h"
#include "../events/event_system.h"
#include "widget_manager.h"
#include "widget_factory.h"
#include "widget.h"
#include "widgets/button_widget.h"
#include "widgets/page_manager_widget.h"
#include "widgets/text_widget.h"
#include "widgets/time_widget.h"
#include "widgets/data_display_widget.h"
#include "page_widget.h"
#include "../core/sdl_includes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "core/logger.h"

void widget_integration_create_shadow_widgets(WidgetIntegration* integration) {
    if (!integration || !integration->widget_manager || integration->shadow_widgets_created) {
        return;
    }
    
    log_info("Creating shadow widgets to mirror existing UI structure");
    
    // Create page manager widget
    integration->page_manager = page_manager_widget_create("page_manager", integration->num_pages);
    if (!integration->page_manager) {
        log_error("Failed to create page manager widget");
        return;
    }
    
    widget_set_bounds(integration->page_manager, 0, 0, integration->screen_width, integration->screen_height);
    widget_manager_add_root(integration->widget_manager, integration->page_manager, "page_manager");
    
    // Set up page change callback to mirror to existing system
    page_manager_set_page_changed_callback(integration->page_manager,
        widget_integration_page_changed_callback, integration);
    
    // Create page widgets for each page
    for (int i = 0; i < integration->num_pages; i++) {
        char page_id[32];
        char page_title[64];
        snprintf(page_id, sizeof(page_id), "page_%d", i);
        snprintf(page_title, sizeof(page_title), "Page %d", i + 1);
        
        // Create a page widget
        Widget* page = widget_create(page_id, WIDGET_TYPE_CONTAINER);
        if (page) {
            widget_set_bounds(page, 0, 0, integration->screen_width, integration->screen_height);
            integration->page_widgets[i] = page;
            
            // Add to page manager instead of directly to widget manager
            page_manager_add_page(integration->page_manager, i, page);
            
            log_debug("Created shadow page widget: %s", page_id);
        }
    }
    
    // Create button widgets for existing buttons
    // Page 0 (Page 1 in UI) has 1 button
    // Page 1 (Page 2 in UI) has up to 9 buttons
    
    // Page 0 button
    if (integration->page_widgets[0]) {
        Widget* button = widget_factory_create_widget(integration->widget_factory,
                                                    "button", "page0_button0", NULL);
        if (button) {
            widget_set_relative_bounds(button, 20, 100, 200, 50);
            
            // Create text widget as child of button
            Widget* label = (Widget*)text_widget_create("page0_button0_text", "Change Text Color", integration->font_regular);
            if (label) {
                // Text fills button area minus padding (relative to button)
                int padding = button->padding;
                widget_set_relative_bounds(label, 
                    padding,
                    padding,
                    button->bounds.w - padding * 2,
                    button->bounds.h - padding * 2);
                text_widget_set_alignment(label, TEXT_ALIGN_CENTER);
                widget_add_child(button, label);
            }
            
            widget_add_child(integration->page_widgets[0], button);
            integration->button_widgets[0][0] = button;
            
            // Configure button to publish events
            if (button->type == WIDGET_TYPE_BUTTON) {
                ButtonWidget* btn = (ButtonWidget*)button;
                btn->base.event_system = integration->event_system;
                
                // Set up event data for this button
                struct {
                    int button_index;
                    int page;
                    uint32_t timestamp;
                    char button_text[32];
                } *click_data = malloc(sizeof(*click_data));
                
                if (click_data) {
                    click_data->button_index = 0;
                    click_data->page = 0;
                    click_data->timestamp = 0; // Will be set when clicked
                    strncpy(click_data->button_text, "Change Text Color", sizeof(click_data->button_text) - 1);
                    log_debug("Setting up button page0_button0: index=0 page=0 text='Change Text Color'");
                    button_widget_set_publish_event(btn, "ui.button_pressed", 
                                                  click_data, sizeof(*click_data));
                }
            }
            
            log_debug("Created shadow button widget: page0_button0 with text");
        }
    }
    
    // Page 1 buttons (color buttons)
    if (integration->page_widgets[1]) {
        const char* button_labels[] = {
            "Blue", "Random", "Time", "Go to Page 1", "Refresh User", 
            "Exit App", "Button 7", "Button 8", "Button 9"
        };
        
        for (int i = 0; i < 9; i++) {
            if (strlen(button_labels[i]) > 0) {
                char button_id[32];
                snprintf(button_id, sizeof(button_id), "page1_button%d", i);
                
                Widget* button = widget_factory_create_widget(integration->widget_factory,
                                                            "button", button_id, NULL);
                if (button) {
                    // Calculate position based on 3x3 grid
                    int row = i / 3;
                    int col = i % 3;
                    int button_width = integration->screen_width / 3 - 20;
                    int button_height = integration->screen_height / 3 - 20;
                    int x = col * (button_width + 10) + 10;
                    int y = row * (button_height + 10) + 10;
                    
                    widget_set_relative_bounds(button, x, y, button_width, button_height);
                    
                    // Create text widget as child of button
                    char text_id[64];
                    snprintf(text_id, sizeof(text_id), "%s_text", button_id);
                    Widget* label = (Widget*)text_widget_create(text_id, button_labels[i], integration->font_regular);
                    if (label) {
                        // Text fills button area minus padding (relative to button)
                        int padding = button->padding;
                        widget_set_relative_bounds(label,
                            padding,
                            padding,
                            button->bounds.w - padding * 2,
                            button->bounds.h - padding * 2);
                        text_widget_set_alignment(label, TEXT_ALIGN_CENTER);
                        widget_add_child(button, label);
                    }
                    
                    widget_add_child(integration->page_widgets[1], button);
                    integration->button_widgets[1][i] = button;
                    
                    // Configure button to publish events
                    if (button->type == WIDGET_TYPE_BUTTON) {
                        ButtonWidget* btn = (ButtonWidget*)button;
                        btn->base.event_system = integration->event_system;
                        
                        // Set up event data for this button
                        struct {
                            int button_index;
                            int page;
                            uint32_t timestamp;
                            char button_text[32];
                        } *click_data = malloc(sizeof(*click_data));
                        
                        if (click_data) {
                            click_data->button_index = i;
                            click_data->page = 1;
                            click_data->timestamp = 0; // Will be set when clicked
                            strncpy(click_data->button_text, button_labels[i], sizeof(click_data->button_text) - 1);
                            log_debug("Setting up button %s: index=%d page=1 text='%s'", 
                                     button_id, i, button_labels[i]);
                            button_widget_set_publish_event(btn, "ui.button_pressed", 
                                                          click_data, sizeof(*click_data));
                        }
                    }
                    
                    log_debug("Created shadow button widget: %s with text", button_id);
                }
            }
        }
    }
    
    // Set page manager as the active root - it will handle page switching
    if (integration->widget_manager && integration->page_manager) {
        widget_manager_set_active_root(integration->widget_manager, "page_manager");
    }
    
    // Add UI widgets to pages
    widget_integration_populate_page_widgets(integration);
    
    integration->shadow_widgets_created = true;
    log_info("Shadow widget tree created successfully");
}

void widget_integration_sync_button_state(WidgetIntegration* integration, 
                                        int page, int button_index, 
                                        const char* text, bool enabled) {
    if (!integration || !integration->shadow_widgets_created || 
        page < 0 || page >= integration->num_pages ||
        button_index < 0 || button_index >= 9) {
        return;
    }
    
    Widget* button = integration->button_widgets[page][button_index];
    if (button && button->type == WIDGET_TYPE_BUTTON) {
        ButtonWidget* btn = (ButtonWidget*)button;
        
        // Update button text if provided by updating the text widget child
        if (text && button->child_count > 0) {
            // Find the text widget child (should be the first child)
            for (size_t i = 0; i < button->child_count; i++) {
                Widget* child = button->children[i];
                if (child && child->type == WIDGET_TYPE_LABEL) {
                    text_widget_set_text(child, text);
                    break;
                }
            }
            
            // Also update the publish data to reflect new text
            if (btn->publish_data && btn->publish_data_size >= sizeof(struct {int button_index; int page; uint32_t timestamp; char button_text[32];})) {
                struct {
                    int button_index;
                    int page;
                    uint32_t timestamp;
                    char button_text[32];
                } *click_data = (void*)btn->publish_data;
                strncpy(click_data->button_text, text, sizeof(click_data->button_text) - 1);
                click_data->button_text[sizeof(click_data->button_text) - 1] = '\0';
            }
        }
        
        // Update enabled state
        widget_set_enabled(button, enabled);
        
        log_debug("Synced button state: page=%d button=%d text='%s' enabled=%d",
                 page, button_index, text ? text : "", enabled);
    }
}

void widget_integration_sync_page_state(WidgetIntegration* integration,
                                       int page_index, bool is_active) {
    if (!integration || !integration->shadow_widgets_created || 
        !integration->page_manager ||
        page_index < 0 || page_index >= integration->num_pages) {
        return;
    }
    
    if (is_active && integration->page_manager) {
        // Sync page state to page manager widget
        if (page_manager_get_current_page(integration->page_manager) != page_index) {
            page_manager_set_current_page(integration->page_manager, page_index);
        }
        log_debug("Synced page state to page manager: page %d", page_index);
    }
}

Widget* widget_integration_get_page_widget(WidgetIntegration* integration, int page) {
    if (!integration || page < 0 || page >= integration->num_pages) {
        return NULL;
    }
    return integration->page_widgets[page];
}

Widget* widget_integration_get_button_widget(WidgetIntegration* integration, int page, int button) {
    if (!integration || page < 0 || page >= integration->num_pages ||
        button < 0 || button >= 9) {
        return NULL;
    }
    return integration->button_widgets[page][button];
}

// Populate pages with actual UI widgets
void widget_integration_populate_page_widgets(WidgetIntegration* integration) {
    if (!integration || !integration->renderer) return;
    
    // Ensure fonts are set before creating widgets
    if (!integration->font_regular || !integration->font_large || !integration->font_small) {
        log_error("Fonts not set! Call widget_integration_set_fonts() first");
        return;
    }
    
    // Populate Page 0 (Welcome page)
    if (integration->page_widgets[0]) {
        Widget* page = integration->page_widgets[0];
        
        // Title text
        Widget* title = (Widget*)text_widget_create("page0_title", "Welcome to PanelKit!", integration->font_large);
        if (title) {
            widget_set_bounds(title, 0, 60, integration->screen_width, 40);
            text_widget_set_alignment(title, TEXT_ALIGN_CENTER);
            widget_add_child(page, title);
        }
        
        // Welcome message (will be updated from state)
        Widget* welcome_text = (Widget*)text_widget_create("page0_welcome", 
            "Welcome to Page 1!", integration->font_regular);
        if (welcome_text) {
            widget_set_bounds(welcome_text, 0, 280, integration->screen_width, 30);
            text_widget_set_alignment(welcome_text, TEXT_ALIGN_CENTER);
            widget_add_child(page, welcome_text);
        }
        
        Widget* instruction_text = (Widget*)text_widget_create("page0_instruction", 
            "Swipe right to see buttons.", integration->font_regular);
        if (instruction_text) {
            widget_set_bounds(instruction_text, 0, 310, integration->screen_width, 30);
            text_widget_set_alignment(instruction_text, TEXT_ALIGN_CENTER);
            widget_add_child(page, instruction_text);
        }
    }
    
    // Populate Page 1 (Buttons and data page)
    if (integration->page_widgets[1]) {
        Widget* page = integration->page_widgets[1];
        
        // Time widget (will be toggled based on show_time)
        Widget* time_widget = (Widget*)time_widget_create("page1_time", "%H:%M:%S", integration->font_large);
        if (time_widget) {
            // Position it where the Time button would show it
            widget_set_bounds(time_widget, 
                integration->screen_width - 150, 10, 140, 40);
            widget_add_child(page, time_widget);
        }
        
        // API data display
        Widget* data_display = (Widget*)data_display_widget_create("page1_data", 
            integration->font_small, integration->font_regular);
        if (data_display) {
            // Position on the right side
            widget_set_bounds(data_display, 
                integration->screen_width / 2, 100, 
                integration->screen_width / 2 - 20, 200);
            widget_add_child(page, data_display);
        }
    }
    
    log_debug("Populated page widgets with UI elements");
}

// Update widget rendering based on state
void widget_integration_update_rendering(WidgetIntegration* integration) {
    if (!integration || !integration->state_store) return;
    
    size_t size;
    time_t timestamp;
    
    // Update page 0 text color
    Widget* welcome_text = widget_manager_find_widget(integration->widget_manager, "page0_welcome");
    if (welcome_text) {
        int* text_color_idx = (int*)state_store_get(integration->state_store, 
                                                   "app", "page1_text_color", &size, &timestamp);
        if (text_color_idx && size == sizeof(int)) {
            // Define text colors
            SDL_Color text_colors[] = {
                {255, 255, 255, 255}, // White
                {255, 100, 100, 255}, // Red
                {100, 255, 100, 255}, // Green
                {100, 100, 255, 255}, // Blue
                {255, 255, 100, 255}, // Yellow
                {255, 100, 255, 255}, // Purple
                {100, 255, 255, 255}, // Cyan
            };
            int idx = *text_color_idx % 7;
            text_widget_set_color(welcome_text, text_colors[idx]);
            free(text_color_idx);
        }
    }
    
    // Update time widget visibility
    Widget* time_widget = widget_manager_find_widget(integration->widget_manager, "page1_time");
    if (time_widget) {
        bool* show_time = (bool*)state_store_get(integration->state_store, 
                                                "app", "show_time", &size, &timestamp);
        if (show_time && size == sizeof(bool)) {
            if (*show_time) {
                time_widget->state_flags &= ~WIDGET_STATE_HIDDEN;
            } else {
                time_widget->state_flags |= WIDGET_STATE_HIDDEN;
            }
            free(show_time);
        }
    }
    
    // Update API data display
    Widget* data_display = widget_manager_find_widget(integration->widget_manager, "page1_data");
    if (data_display && integration->state_tracking_enabled) {
        void* user_data = state_store_get(integration->state_store, "api_data", "user", &size, &timestamp);
        if (user_data && size > 0) {
            // Parse user data (simplified for now)
            // In real implementation, properly deserialize UserData struct
            data_display_widget_set_user_data(data_display, "John Doe", 
                "john@example.com", "555-1234", "New York", "USA");
            free(user_data);
        } else {
            data_display_widget_clear(data_display);
        }
    }
}