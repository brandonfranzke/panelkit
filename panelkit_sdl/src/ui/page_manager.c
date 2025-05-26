#include "page_manager.h"
#include "widget_factory.h"
#include "pages/home_page.h"
#include "pages/settings_page.h"
#include "../events/event_system.h"
#include "../state/state_store.h"
#include "../config/config_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Simple logging macros
#ifndef log_info
#define log_info(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

// Navigation event handler
static void handle_navigation_event(const char* event_name,
                                  const void* data,
                                  size_t data_size,
                                  void* context) {
    PageManager* manager = (PageManager*)context;
    
    if (strcmp(event_name, "navigation.goto") == 0 && data_size >= sizeof(int)) {
        const struct { int target_page; } *nav_data = data;
        page_manager_goto_page(manager, nav_data->target_page);
    }
}

PageManager* page_manager_create(SDL_Renderer* renderer,
                               EventSystem* event_system,
                               StateStore* state_store,
                               ConfigManager* config_manager) {
    PageManager* manager = calloc(1, sizeof(PageManager));
    if (!manager) {
        log_error("Failed to allocate page manager");
        return NULL;
    }
    
    manager->renderer = renderer;
    manager->event_system = event_system;
    manager->state_store = state_store;
    manager->config_manager = config_manager;
    
    // Create widget manager
    manager->widget_manager = widget_manager_create(renderer, event_system, state_store);
    if (!manager->widget_manager) {
        free(manager);
        return NULL;
    }
    
    // Create widget factory
    manager->widget_factory = widget_factory_create_default();
    if (!manager->widget_factory) {
        widget_manager_destroy(manager->widget_manager);
        free(manager);
        return NULL;
    }
    
    // Initialize state
    manager->current_page = 0;
    manager->total_pages = 0;
    manager->transition_state = TRANSITION_NONE;
    manager->transition_offset = 0.0f;
    manager->target_page = -1;
    manager->show_indicators = false;
    manager->indicator_hide_time = 0;
    
    // Subscribe to navigation events
    if (event_system) {
        event_subscribe(event_system, "navigation.goto", 
                       handle_navigation_event, manager);
    }
    
    log_info("Created page manager");
    return manager;
}

void page_manager_destroy(PageManager* manager) {
    if (!manager) {
        return;
    }
    
    // Unsubscribe from events
    if (manager->event_system) {
        event_unsubscribe(manager->event_system, "navigation.goto",
                         handle_navigation_event);
    }
    
    widget_factory_destroy(manager->widget_factory);
    widget_manager_destroy(manager->widget_manager);
    free(manager);
    
    log_info("Destroyed page manager");
}

bool page_manager_init_pages(PageManager* manager) {
    if (!manager) {
        return false;
    }
    
    // Create home page
    HomePage* home_page = home_page_create(manager->widget_factory,
                                         manager->event_system,
                                         manager->state_store,
                                         manager->config_manager);
    if (!home_page) {
        log_error("Failed to create home page");
        return false;
    }
    
    // Set bounds to full screen
    widget_set_bounds(&home_page->base.base, 0, 0, 
                     manager->screen_width, manager->screen_height);
    
    // Add to widget manager
    widget_manager_add_root(manager->widget_manager, 
                          &home_page->base.base, "home");
    
    // Create settings page
    SettingsPage* settings_page = settings_page_create(manager->widget_factory,
                                                     manager->event_system,
                                                     manager->state_store,
                                                     manager->config_manager);
    if (!settings_page) {
        log_error("Failed to create settings page");
        return false;
    }
    
    // Set bounds to full screen
    widget_set_bounds(&settings_page->base.base, 0, 0,
                     manager->screen_width, manager->screen_height);
    
    // Add to widget manager
    widget_manager_add_root(manager->widget_manager,
                          &settings_page->base.base, "settings");
    
    manager->total_pages = 2;
    manager->current_page = 0;
    
    // Set home as active
    widget_manager_set_active_root(manager->widget_manager, "home");
    
    log_info("Initialized %d pages", manager->total_pages);
    return true;
}

void page_manager_set_dimensions(PageManager* manager, int width, int height) {
    if (!manager) {
        return;
    }
    
    manager->screen_width = width;
    manager->screen_height = height;
    
    // Update all page sizes
    for (int i = 0; i < manager->total_pages; i++) {
        const char* page_names[] = {"home", "settings"};
        Widget* page = widget_manager_get_root(manager->widget_manager, page_names[i]);
        if (page) {
            widget_set_bounds(page, 0, 0, width, height);
        }
    }
}

void page_manager_handle_event(PageManager* manager, const SDL_Event* event) {
    if (!manager) {
        return;
    }
    
    // Let widget manager handle the event
    widget_manager_handle_event(manager->widget_manager, event);
}

void page_manager_handle_drag(PageManager* manager, float offset, bool is_complete) {
    if (!manager) {
        return;
    }
    
    if (!is_complete) {
        // Still dragging
        manager->transition_state = TRANSITION_DRAGGING;
        manager->transition_offset = offset / manager->screen_width;
        
        // Show indicators
        manager->show_indicators = true;
        manager->indicator_hide_time = SDL_GetTicks() + 2000;
    } else {
        // Drag complete - determine if we should change pages
        float threshold = 0.3f;
        
        if (fabs(manager->transition_offset) > threshold) {
            // Change page
            if (manager->transition_offset < 0 && manager->current_page < manager->total_pages - 1) {
                // Swipe left - next page
                page_manager_goto_page(manager, manager->current_page + 1);
            } else if (manager->transition_offset > 0 && manager->current_page > 0) {
                // Swipe right - previous page
                page_manager_goto_page(manager, manager->current_page - 1);
            } else {
                // Can't go further, snap back
                manager->transition_state = TRANSITION_ANIMATING;
                manager->target_page = manager->current_page;
            }
        } else {
            // Not enough movement, snap back
            manager->transition_state = TRANSITION_ANIMATING;
            manager->target_page = manager->current_page;
        }
    }
}

void page_manager_handle_swipe(PageManager* manager, bool is_horizontal, float velocity) {
    if (!manager || !is_horizontal) {
        return;
    }
    
    // Quick swipe can change pages with less distance
    float velocity_threshold = 500.0f;
    
    if (fabs(velocity) > velocity_threshold) {
        if (velocity < 0 && manager->current_page < manager->total_pages - 1) {
            // Fast swipe left
            page_manager_goto_page(manager, manager->current_page + 1);
        } else if (velocity > 0 && manager->current_page > 0) {
            // Fast swipe right
            page_manager_goto_page(manager, manager->current_page - 1);
        }
    }
}

void page_manager_update(PageManager* manager) {
    if (!manager) {
        return;
    }
    
    // Update transition animation
    if (manager->transition_state == TRANSITION_ANIMATING && manager->target_page >= 0) {
        float animation_speed = 5.0f;
        float target = (manager->target_page == manager->current_page) ? 0.0f : 1.0f;
        
        float diff = target - fabs(manager->transition_offset);
        manager->transition_offset += diff * animation_speed * 0.016f;  // 60 FPS
        
        if (fabs(diff) < 0.01f) {
            // Animation complete
            if (manager->target_page != manager->current_page) {
                // Switch pages
                manager->current_page = manager->target_page;
                const char* page_names[] = {"home", "settings"};
                widget_manager_set_active_root(manager->widget_manager,
                                             page_names[manager->current_page]);
            }
            
            manager->transition_state = TRANSITION_NONE;
            manager->transition_offset = 0.0f;
            manager->target_page = -1;
        }
    }
    
    // Update indicator fade
    if (manager->show_indicators && SDL_GetTicks() > manager->indicator_hide_time) {
        manager->show_indicators = false;
    }
    
    // Update widget manager
    widget_manager_update(manager->widget_manager);
}

void page_manager_render(PageManager* manager) {
    if (!manager) {
        return;
    }
    
    // Render based on transition state
    if (manager->transition_state != TRANSITION_NONE && 
        fabs(manager->transition_offset) > 0.001f) {
        
        // Rendering transition between pages
        // For now, just render the current page with offset
        // In a full implementation, we'd render both pages with appropriate offsets
        
        SDL_Rect viewport = {
            (int)(manager->transition_offset * manager->screen_width),
            0,
            manager->screen_width,
            manager->screen_height
        };
        SDL_RenderSetViewport(manager->renderer, &viewport);
    }
    
    // Render active page
    widget_manager_render(manager->widget_manager);
    
    // Reset viewport
    SDL_RenderSetViewport(manager->renderer, NULL);
    
    // Render page indicators
    if (manager->show_indicators) {
        page_manager_render_indicators(manager);
    }
}

void page_manager_goto_page(PageManager* manager, int page_index) {
    if (!manager || page_index < 0 || page_index >= manager->total_pages) {
        return;
    }
    
    if (page_index != manager->current_page) {
        manager->target_page = page_index;
        manager->transition_state = TRANSITION_ANIMATING;
        
        log_info("Navigating from page %d to page %d", 
                manager->current_page, page_index);
    }
}

int page_manager_get_current_page(PageManager* manager) {
    return manager ? manager->current_page : 0;
}

void page_manager_render_indicators(PageManager* manager) {
    if (!manager || manager->total_pages <= 1) {
        return;
    }
    
    int indicator_size = 8;
    int indicator_spacing = 16;
    int total_width = manager->total_pages * indicator_size + 
                     (manager->total_pages - 1) * indicator_spacing;
    
    int start_x = (manager->screen_width - total_width) / 2;
    int y = manager->screen_height - 30;
    
    for (int i = 0; i < manager->total_pages; i++) {
        int x = start_x + i * (indicator_size + indicator_spacing);
        
        SDL_Rect indicator = {x, y, indicator_size, indicator_size};
        
        if (i == manager->current_page) {
            // Active page - filled circle
            SDL_SetRenderDrawColor(manager->renderer, 255, 255, 255, 200);
            SDL_RenderFillRect(manager->renderer, &indicator);
        } else {
            // Inactive page - outline
            SDL_SetRenderDrawColor(manager->renderer, 255, 255, 255, 100);
            SDL_RenderDrawRect(manager->renderer, &indicator);
        }
    }
}