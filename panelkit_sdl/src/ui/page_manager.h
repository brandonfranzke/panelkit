#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "widget_manager.h"
#include "pages.h"
#include <SDL.h>

// Forward declarations
typedef struct WidgetFactory WidgetFactory;
typedef struct EventSystem EventSystem;
typedef struct StateStore StateStore;
typedef struct ConfigManager ConfigManager;

// Page manager - bridges old page system with new widget-based pages
typedef struct PageManager {
    // Widget system
    WidgetManager* widget_manager;
    WidgetFactory* widget_factory;
    
    // Systems
    EventSystem* event_system;
    StateStore* state_store;
    ConfigManager* config_manager;
    
    // Renderer
    SDL_Renderer* renderer;
    
    // Page tracking
    int current_page;
    int total_pages;
    
    // Transition state
    TransitionState transition_state;
    float transition_offset;
    int target_page;
    
    // Screen dimensions
    int screen_width;
    int screen_height;
    
    // Page indicator state
    bool show_indicators;
    Uint32 indicator_hide_time;
} PageManager;

// Lifecycle
PageManager* page_manager_create(SDL_Renderer* renderer,
                               EventSystem* event_system,
                               StateStore* state_store,
                               ConfigManager* config_manager);
void page_manager_destroy(PageManager* manager);

// Initialize pages
bool page_manager_init_pages(PageManager* manager);

// Screen dimensions
void page_manager_set_dimensions(PageManager* manager, int width, int height);

// Event handling
void page_manager_handle_event(PageManager* manager, const SDL_Event* event);

// Gesture handling
void page_manager_handle_drag(PageManager* manager, float offset, bool is_complete);
void page_manager_handle_swipe(PageManager* manager, bool is_horizontal, float velocity);

// Update and render
void page_manager_update(PageManager* manager);
void page_manager_render(PageManager* manager);

// Page navigation
void page_manager_goto_page(PageManager* manager, int page_index);
int page_manager_get_current_page(PageManager* manager);

// Page indicators
void page_manager_render_indicators(PageManager* manager);

#endif // PAGE_MANAGER_H