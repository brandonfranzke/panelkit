#include "page_manager_widget.h"
#include "../widget.h"
#include "../widget_manager.h"
#include "../../state/state_store.h"
#include "../../events/event_system.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "core/logger.h"
#include "core/error.h"

// Animation constants
#define TRANSITION_SPEED 0.12f
#define SWIPE_THRESHOLD 0.3f  // 30% of screen width
#define ELASTIC_RESISTANCE 0.3f
#define INDICATOR_FADE_DURATION 400
#define INDICATOR_HIDE_DELAY 2000
#define INDICATOR_DEFAULT_ALPHA 153

// Forward declarations
static void page_manager_update(Widget* widget, double delta_time);
static PkError page_manager_render(Widget* widget, SDL_Renderer* renderer);
static void page_manager_handle_event(Widget* widget, const SDL_Event* event);
static void page_manager_destroy(Widget* widget);

// Create a new page manager widget
Widget* page_manager_widget_create(const char* id, int page_count) {
    if (!id) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "id is NULL in page_manager_widget_create");
        return NULL;
    }
    if (page_count <= 0) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_ARGUMENT,
                                       "page_count must be positive in page_manager_widget_create");
        return NULL;
    }
    
    PageManagerWidget* manager = calloc(1, sizeof(PageManagerWidget));
    if (!manager) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate page manager widget");
        return NULL;
    }
    
    // Initialize base widget
    Widget* base = &manager->base;
    strncpy(base->id, id, sizeof(base->id) - 1);
    base->type = WIDGET_TYPE_CONTAINER;
    base->state_flags = WIDGET_STATE_NORMAL;
    
    // Set widget methods
    base->update = page_manager_update;
    base->render = page_manager_render;
    base->handle_event = page_manager_handle_event;
    base->destroy = page_manager_destroy;
    
    // Initialize widget arrays 
    base->child_capacity = page_count;
    base->children = calloc(base->child_capacity, sizeof(Widget*));
    if (!base->children) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate children array for page manager");
        free(manager->pages);
        free(manager);
        return NULL;
    }
    
    base->event_capacity = 4;
    base->subscribed_events = calloc(base->event_capacity, sizeof(char*));
    if (!base->subscribed_events) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate event array for page manager");
        free(base->children);
        free(manager->pages);
        free(manager);
        return NULL;
    }
    
    // Initialize page management
    manager->page_count = page_count;
    manager->pages = calloc(page_count, sizeof(Widget*));
    if (!manager->pages) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate pages array for page manager");
        free(base->subscribed_events);
        free(base->children);
        free(manager);
        return NULL;
    }
    manager->current_page = 0;
    manager->target_page = -1;
    manager->transition_state = PAGE_TRANSITION_NONE;
    manager->transition_offset = 0.0f;
    manager->drag_offset = 0.0f;
    
    // Initialize indicators
    manager->show_indicators = true;
    manager->indicator_alpha = 0;
    manager->indicator_hide_time = 0;
    manager->last_interaction_time = SDL_GetTicks();
    
    return (Widget*)manager;
}

// Add a page widget
void page_manager_add_page(Widget* widget, int index, Widget* page) {
    if (!widget || widget->type != WIDGET_TYPE_CONTAINER) return;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    if (index < 0 || index >= manager->page_count || !page) return;
    
    // Remove old page if exists
    if (manager->pages[index]) {
        widget_remove_child(widget, manager->pages[index]);
    }
    
    // Add new page
    manager->pages[index] = page;
    widget_add_child(widget, page);
    
    // Position page based on index
    page->bounds.x = widget->bounds.x + index * widget->bounds.w;
    page->bounds.y = widget->bounds.y;
    page->bounds.w = widget->bounds.w;
    page->bounds.h = widget->bounds.h;
}

// Set current page
void page_manager_set_current_page(Widget* widget, int page_index) {
    if (!widget || widget->type != WIDGET_TYPE_CONTAINER) return;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    if (page_index < 0 || page_index >= manager->page_count) return;
    
    int old_page = manager->current_page;
    manager->current_page = page_index;
    manager->transition_offset = 0.0f;
    manager->transition_state = PAGE_TRANSITION_NONE;
    
    // Notify callback
    if (manager->on_page_changed && old_page != page_index) {
        manager->on_page_changed(old_page, page_index, manager->callback_user_data);
    }
    
    // Update state store through global references (temporary)
    // TODO: Get state store reference properly
}

// Get current page
int page_manager_get_current_page(Widget* widget) {
    if (!widget || widget->type != WIDGET_TYPE_CONTAINER) return 0;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    return manager->current_page;
}

// Start transition to page
void page_manager_transition_to(Widget* widget, int page_index) {
    if (!widget || widget->type != WIDGET_TYPE_CONTAINER) return;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    if (page_index < 0 || page_index >= manager->page_count) return;
    if (page_index == manager->current_page) return;
    if (manager->transition_state == PAGE_TRANSITION_ANIMATING) return;
    
    manager->target_page = page_index;
    manager->transition_state = PAGE_TRANSITION_ANIMATING;
    manager->transition_offset = 0.0f;
    manager->show_indicators = true;
    manager->indicator_alpha = INDICATOR_DEFAULT_ALPHA;
    manager->last_interaction_time = SDL_GetTicks();
}

// Handle swipe gestures
void page_manager_handle_swipe(Widget* widget, float offset, bool is_complete) {
    if (!widget || widget->type != WIDGET_TYPE_CONTAINER) return;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    if (is_complete) {
        // Swipe complete - determine if we should change pages
        float normalized_offset = offset / widget->bounds.w;
        float abs_normalized = fabsf(normalized_offset);
        
        log_debug("Swipe complete: offset=%.1f normalized=%.3f threshold=%.1f", 
                 offset, normalized_offset, SWIPE_THRESHOLD);
        
        if (abs_normalized > SWIPE_THRESHOLD && manager->transition_state == PAGE_TRANSITION_DRAGGING) {
            // Change page
            int new_page = manager->current_page;
            if (normalized_offset < 0 && manager->current_page < manager->page_count - 1) {
                new_page = manager->current_page + 1;
            } else if (normalized_offset > 0 && manager->current_page > 0) {
                new_page = manager->current_page - 1;
            }
            
            if (new_page != manager->current_page) {
                manager->target_page = new_page;
                manager->transition_state = PAGE_TRANSITION_ANIMATING;
            } else {
                // Snap back
                manager->target_page = -1;
                manager->transition_state = PAGE_TRANSITION_ANIMATING;
            }
        } else {
            // Snap back
            manager->target_page = -1;
            manager->transition_state = PAGE_TRANSITION_ANIMATING;
        }
    } else {
        // Dragging - normalize offset to page width
        float normalized_offset = offset / widget->bounds.w;
        manager->transition_state = PAGE_TRANSITION_DRAGGING;
        manager->transition_offset = normalized_offset;
        manager->drag_offset = normalized_offset;
        
        // Apply elastic resistance at boundaries
        if ((manager->current_page == 0 && normalized_offset > 0) ||
            (manager->current_page == manager->page_count - 1 && normalized_offset < 0)) {
            manager->transition_offset *= ELASTIC_RESISTANCE;
        }
        
        log_debug("Page drag: offset=%.1f normalized=%.3f transition=%.3f", 
                 offset, normalized_offset, manager->transition_offset);
    }
    
    // Show indicators while interacting
    manager->show_indicators = true;
    manager->indicator_alpha = INDICATOR_DEFAULT_ALPHA;
    manager->last_interaction_time = SDL_GetTicks();
}

// Update drag offset
void page_manager_update_drag(Widget* widget, float delta_x) {
    if (!widget || widget->type != WIDGET_TYPE_CONTAINER) return;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    if (manager->transition_state != PAGE_TRANSITION_DRAGGING) {
        manager->transition_state = PAGE_TRANSITION_DRAGGING;
    }
    
    manager->drag_offset += delta_x / widget->bounds.w;
    page_manager_handle_swipe(widget, manager->drag_offset, false);
}

// Set page change callback
void page_manager_set_page_changed_callback(Widget* widget, 
    void (*callback)(int from, int to, void* user_data), void* user_data) {
    if (!widget || widget->type != WIDGET_TYPE_CONTAINER) return;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    manager->on_page_changed = callback;
    manager->callback_user_data = user_data;
}

// Update function
static void page_manager_update(Widget* widget, double delta_time) {
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    // Handle transition animation
    if (manager->transition_state == PAGE_TRANSITION_ANIMATING) {
        if (manager->target_page >= 0) {
            // Transitioning to a new page
            float target_offset = (manager->target_page > manager->current_page) ? -1.0f : 1.0f;
            float diff = target_offset - manager->transition_offset;
            
            manager->transition_offset += diff * TRANSITION_SPEED;
            
            // Check if transition complete
            if (fabsf(diff) < 0.01f) {
                page_manager_set_current_page(widget, manager->target_page);
                manager->target_page = -1;
            }
        } else {
            // Snapping back to current page
            manager->transition_offset *= (1.0f - TRANSITION_SPEED);
            
            if (fabsf(manager->transition_offset) < 0.01f) {
                manager->transition_offset = 0.0f;
                manager->transition_state = PAGE_TRANSITION_NONE;
                manager->drag_offset = 0.0f;
            }
        }
    }
    
    // Handle indicator fade
    uint32_t current_time = SDL_GetTicks();
    if (manager->show_indicators && manager->indicator_alpha > 0) {
        uint32_t time_since_interaction = current_time - manager->last_interaction_time;
        
        if (time_since_interaction > INDICATOR_HIDE_DELAY) {
            // Start fading
            uint32_t fade_time = time_since_interaction - INDICATOR_HIDE_DELAY;
            if (fade_time < INDICATOR_FADE_DURATION) {
                float fade_progress = (float)fade_time / INDICATOR_FADE_DURATION;
                manager->indicator_alpha = (int)(INDICATOR_DEFAULT_ALPHA * (1.0f - fade_progress));
            } else {
                manager->indicator_alpha = 0;
                manager->show_indicators = false;
            }
        }
    }
    
    // Update child pages positions based on transition
    for (int i = 0; i < manager->page_count; i++) {
        if (manager->pages[i]) {
            float page_offset = (i - manager->current_page) + manager->transition_offset;
            int new_x = widget->bounds.x + (int)(page_offset * widget->bounds.w);
            int new_y = widget->bounds.y;
            
            // Update page position
            manager->pages[i]->bounds.x = new_x;
            manager->pages[i]->bounds.y = new_y;
            
            // Update all child widget positions recursively
            widget_update_child_bounds(manager->pages[i]);
        }
    }
}

// Render function
static PkError page_manager_render(Widget* widget, SDL_Renderer* renderer) {
    PK_CHECK_ERROR_WITH_CONTEXT(widget != NULL, PK_ERROR_NULL_PARAM,
                               "widget is NULL in page_manager_render");
    PK_CHECK_ERROR_WITH_CONTEXT(renderer != NULL, PK_ERROR_NULL_PARAM,
                               "renderer is NULL in page_manager_render");
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    log_debug("PAGE MANAGER RENDER: %d pages, current=%d, bounds=(%d,%d,%dx%d)", 
              manager->page_count, manager->current_page,
              widget->bounds.x, widget->bounds.y, widget->bounds.w, widget->bounds.h);
    
    // Set clipping rectangle
    SDL_RenderSetClipRect(renderer, &widget->bounds);
    
    // Render visible pages
    for (int i = 0; i < manager->page_count; i++) {
        if (manager->pages[i]) {
            log_debug("  Page %d: id=%s, hidden=%d, x=%d, render=%p", 
                      i, manager->pages[i]->id,
                      (manager->pages[i]->state_flags & WIDGET_STATE_HIDDEN) ? 1 : 0,
                      manager->pages[i]->bounds.x,
                      manager->pages[i]->render);
            
            if (!(manager->pages[i]->state_flags & WIDGET_STATE_HIDDEN)) {
                // Check if page is visible (any part of it is within the viewport)
                int page_x = manager->pages[i]->bounds.x;
                int page_right = page_x + manager->pages[i]->bounds.w;
                int viewport_left = widget->bounds.x;
                int viewport_right = widget->bounds.x + widget->bounds.w;
                
                if (page_right > viewport_left && page_x < viewport_right) {
                    log_debug("    Page %d is visible, calling render", i);
                    if (manager->pages[i]->render) {
                        PkError err = manager->pages[i]->render(manager->pages[i], renderer);
                        if (err != PK_OK) {
                            SDL_RenderSetClipRect(renderer, NULL);
                            return err;
                        }
                    }
                } else {
                    log_debug("    Page %d NOT visible: page_x=%d, check failed", i, page_x);
                }
            }
        } else {
            log_debug("  Page %d: NULL", i);
        }
    }
    
    // Render page indicators if visible
    if (manager->show_indicators && manager->indicator_alpha > 0) {
        int indicator_size = 8;
        int indicator_spacing = 16;
        int total_width = manager->page_count * indicator_spacing - (indicator_spacing - indicator_size);
        int start_x = widget->bounds.x + (widget->bounds.w - total_width) / 2;
        int y = widget->bounds.y + widget->bounds.h - 30;
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        for (int i = 0; i < manager->page_count; i++) {
            int x = start_x + i * indicator_spacing;
            
            // Determine indicator state
            float highlight = 0.0f;
            if (i == manager->current_page) {
                highlight = 1.0f - fabsf(manager->transition_offset);
            } else if (i == manager->target_page && manager->target_page >= 0) {
                highlight = fabsf(manager->transition_offset);
            }
            
            // Draw indicator
            int alpha = (int)(manager->indicator_alpha * (0.3f + 0.7f * highlight));
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
            
            SDL_Rect indicator = {x, y, indicator_size, indicator_size};
            if (SDL_RenderFillRect(renderer, &indicator) < 0) {
                pk_set_last_error_with_context(PK_ERROR_RENDER_FAILED,
                                               "Failed to draw page indicator: %s",
                                               SDL_GetError());
                SDL_RenderSetClipRect(renderer, NULL);
                return PK_ERROR_RENDER_FAILED;
            }
        }
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    // Clear clipping
    SDL_RenderSetClipRect(renderer, NULL);
    
    return PK_OK;
}

// Handle events
static void page_manager_handle_event(Widget* widget, const SDL_Event* event) {
    if (!widget || !event) return;
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    // Track drag state
    static bool is_dragging = false;
    static int drag_start_x = 0;
    static int drag_page_start = 0;
    
    // Don't handle events if we're not the target of a drag
    if (!is_dragging && event->type != SDL_MOUSEBUTTONDOWN && event->type != SDL_FINGERDOWN) {
        return;
    }
    
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_FINGERDOWN: {
            int x = (event->type == SDL_MOUSEBUTTONDOWN) ? 
                    event->button.x : 
                    (int)(event->tfinger.x * widget->bounds.w);
            
            if (widget_contains_point(widget, x, 
                (event->type == SDL_MOUSEBUTTONDOWN) ? event->button.y : 
                (int)(event->tfinger.y * widget->bounds.h))) {
                is_dragging = true;
                drag_start_x = x;
                drag_page_start = manager->current_page;
                manager->drag_offset = 0.0f;
                manager->transition_state = PAGE_TRANSITION_DRAGGING;
                
                // Show indicators
                manager->show_indicators = true;
                manager->indicator_alpha = INDICATOR_DEFAULT_ALPHA;
                manager->last_interaction_time = SDL_GetTicks();
            }
            break;
        }
        
        case SDL_MOUSEMOTION:
        case SDL_FINGERMOTION: {
            if (is_dragging) {
                int x = (event->type == SDL_MOUSEMOTION) ? 
                        event->motion.x : 
                        (int)(event->tfinger.x * widget->bounds.w);
                
                float delta_x = (float)(x - drag_start_x);
                log_debug("Page manager drag: delta_x=%.1f, start_x=%d, current_x=%d", 
                         delta_x, drag_start_x, x);
                page_manager_handle_swipe(widget, delta_x, false);
            }
            break;
        }
        
        case SDL_MOUSEBUTTONUP:
        case SDL_FINGERUP: {
            if (is_dragging) {
                int x = (event->type == SDL_MOUSEBUTTONUP) ? 
                        event->button.x : 
                        (int)(event->tfinger.x * widget->bounds.w);
                
                float delta_x = (float)(x - drag_start_x);
                page_manager_handle_swipe(widget, delta_x, true);
                is_dragging = false;
            }
            break;
        }
    }
}

// Destroy function
static void page_manager_destroy(Widget* widget) {
    PageManagerWidget* manager = (PageManagerWidget*)widget;
    
    // Pages are destroyed by widget_destroy_children
    if (manager->pages) {
        free(manager->pages);
    }
    
    // Base cleanup happens in widget_destroy
}