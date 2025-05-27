#include "page_widget.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"

// Forward declarations for virtual functions
static PkError page_widget_render(Widget* widget, SDL_Renderer* renderer);
static void page_widget_handle_event(Widget* widget, const SDL_Event* event);
static void page_widget_layout(Widget* widget);
static void page_widget_destroy(Widget* widget);

PageWidget* page_widget_create(const char* id, const char* title) {
    PageWidget* page = calloc(1, sizeof(PageWidget));
    if (!page) {
        log_error("Failed to allocate page widget");
        return NULL;
    }
    
    // Initialize base widget
    Widget* base = &page->base;
    strncpy(base->id, id, sizeof(base->id) - 1);
    base->type = WIDGET_TYPE_CUSTOM;  // Pages are custom widgets
    
    // Set virtual functions
    base->render = page_widget_render;
    base->handle_event = page_widget_handle_event;
    base->layout = page_widget_layout;
    base->destroy = page_widget_destroy;
    
    // Initialize widget arrays
    base->child_capacity = 4;
    base->children = calloc(base->child_capacity, sizeof(Widget*));
    if (!base->children) {
        free(page);
        return NULL;
    }
    
    base->event_capacity = 4;
    base->subscribed_events = calloc(base->event_capacity, sizeof(char*));
    if (!base->subscribed_events) {
        free(base->children);
        free(page);
        return NULL;
    }
    
    // Set default colors (dark theme to match app)
    page->background_color = (SDL_Color){33, 33, 33, 255};
    page->title_color = (SDL_Color){255, 255, 255, 255};
    
    base->background_color = page->background_color;
    base->border_width = 0;
    base->padding = 0;
    
    // Initialize page properties
    strncpy(page->title, title, sizeof(page->title) - 1);
    page->scroll_position = 0;
    page->max_scroll = 0;
    page->content_height = 0;
    
    log_info("Created page widget '%s' with title '%s'", id, title);
    return page;
}

void page_widget_set_title(PageWidget* page, const char* title) {
    if (!page || !title) {
        return;
    }
    
    strncpy(page->title, title, sizeof(page->title) - 1);
    widget_invalidate(&page->base);
}

void page_widget_set_colors(PageWidget* page, SDL_Color background, SDL_Color title_color) {
    if (!page) {
        return;
    }
    
    page->background_color = background;
    page->title_color = title_color;
    page->base.background_color = background;
    widget_invalidate(&page->base);
}

void page_widget_scroll(PageWidget* page, int delta) {
    if (!page) {
        return;
    }
    
    int old_position = page->scroll_position;
    page->scroll_position += delta;
    
    // Clamp to valid range
    if (page->scroll_position < 0) {
        page->scroll_position = 0;
    } else if (page->scroll_position > page->max_scroll) {
        page->scroll_position = page->max_scroll;
    }
    
    if (old_position != page->scroll_position) {
        widget_invalidate(&page->base);
        
        // Call scroll callback
        if (page->on_scroll) {
            page->on_scroll(page, delta);
        }
    }
}

void page_widget_set_scroll_position(PageWidget* page, int position) {
    if (!page) {
        return;
    }
    
    page->scroll_position = position;
    
    // Clamp to valid range
    if (page->scroll_position < 0) {
        page->scroll_position = 0;
    } else if (page->scroll_position > page->max_scroll) {
        page->scroll_position = page->max_scroll;
    }
    
    widget_invalidate(&page->base);
}

int page_widget_get_scroll_position(PageWidget* page) {
    return page ? page->scroll_position : 0;
}

void page_widget_update_content_height(PageWidget* page) {
    if (!page) {
        return;
    }
    
    // Calculate content height from children
    int max_bottom = 0;
    Widget* base = &page->base;
    
    for (size_t i = 0; i < base->child_count; i++) {
        Widget* child = base->children[i];
        if (widget_is_visible(child)) {
            int child_bottom = child->bounds.y + child->bounds.h - base->bounds.y;
            if (child_bottom > max_bottom) {
                max_bottom = child_bottom;
            }
        }
    }
    
    page->content_height = max_bottom + base->padding;
    
    // Update max scroll
    int visible_height = base->bounds.h;
    page->max_scroll = page->content_height - visible_height;
    if (page->max_scroll < 0) {
        page->max_scroll = 0;
    }
    
    // Adjust current scroll if needed
    if (page->scroll_position > page->max_scroll) {
        page->scroll_position = page->max_scroll;
    }
}

void page_widget_set_enter_callback(PageWidget* page, void (*callback)(PageWidget*)) {
    if (page) {
        page->on_enter = callback;
    }
}

void page_widget_set_leave_callback(PageWidget* page, void (*callback)(PageWidget*)) {
    if (page) {
        page->on_leave = callback;
    }
}

void page_widget_set_scroll_callback(PageWidget* page, void (*callback)(PageWidget*, int)) {
    if (page) {
        page->on_scroll = callback;
    }
}

void page_widget_enter(PageWidget* page) {
    if (!page) {
        return;
    }
    
    log_debug("Entering page '%s'", page->base.id);
    
    if (page->on_enter) {
        page->on_enter(page);
    }
    
    widget_invalidate(&page->base);
}

void page_widget_leave(PageWidget* page) {
    if (!page) {
        return;
    }
    
    log_debug("Leaving page '%s'", page->base.id);
    
    if (page->on_leave) {
        page->on_leave(page);
    }
}

Widget* page_widget_create_content_container(PageWidget* page) {
    if (!page) {
        return NULL;
    }
    
    char container_id[128];
    snprintf(container_id, sizeof(container_id), "%s_content", page->base.id);
    
    Widget* container = widget_create(container_id, WIDGET_TYPE_CONTAINER);
    if (!container) {
        return NULL;
    }
    
    // Set container to fill page minus title area
    int title_height = 50;  // Standard title bar height
    widget_set_relative_bounds(container, 
                             0, 
                             title_height,
                             page->base.bounds.w,
                             page->base.bounds.h - title_height);
    
    // Make container transparent
    container->background_color = (SDL_Color){0, 0, 0, 0};
    container->border_width = 0;
    
    widget_add_child(&page->base, container);
    
    return container;
}

static PkError page_widget_render(Widget* widget, SDL_Renderer* renderer) {
    PageWidget* page = (PageWidget*)widget;
    PK_CHECK_ERROR_WITH_CONTEXT(page != NULL, PK_ERROR_NULL_PARAM,
                               "page widget is NULL in page_widget_render");
    PK_CHECK_ERROR_WITH_CONTEXT(renderer != NULL, PK_ERROR_NULL_PARAM,
                               "renderer is NULL in page_widget_render");
    
    log_debug("PAGE RENDER: %s at (%d,%d) size %dx%d bg(%d,%d,%d) children=%zu", 
              widget->id, widget->bounds.x, widget->bounds.y,
              widget->bounds.w, widget->bounds.h,
              page->background_color.r, page->background_color.g, page->background_color.b,
              widget->child_count);
    
    // Draw background
    SDL_SetRenderDrawColor(renderer,
                          page->background_color.r,
                          page->background_color.g,
                          page->background_color.b,
                          page->background_color.a);
    if (SDL_RenderFillRect(renderer, &widget->bounds) < 0) {
        pk_set_last_error_with_context(PK_ERROR_RENDER_FAILED,
                                       "Failed to fill page background: %s",
                                       SDL_GetError());
        return PK_ERROR_RENDER_FAILED;
    }
    
    // Draw title bar
    SDL_Rect title_bar = {
        widget->bounds.x,
        widget->bounds.y,
        widget->bounds.w,
        50  // Standard title height
    };
    
    // Slightly darker title bar
    SDL_SetRenderDrawColor(renderer,
                          page->background_color.r * 0.9,
                          page->background_color.g * 0.9,
                          page->background_color.b * 0.9,
                          page->background_color.a);
    SDL_RenderFillRect(renderer, &title_bar);
    
    // Draw title text indicator
    if (strlen(page->title) > 0) {
        SDL_SetRenderDrawColor(renderer,
                             page->title_color.r,
                             page->title_color.g,
                             page->title_color.b,
                             page->title_color.a);
        
        SDL_Rect title_rect = {
            title_bar.x + 20,
            title_bar.y + 15,
            (int)(strlen(page->title) * 10),
            20
        };
        SDL_RenderDrawRect(renderer, &title_rect);
    }
    
    // Set clip rect for scrollable content
    SDL_Rect content_rect = {
        widget->bounds.x,
        widget->bounds.y + 50,
        widget->bounds.w,
        widget->bounds.h - 50
    };
    SDL_RenderSetClipRect(renderer, &content_rect);
    
    // Apply scroll offset to children
    int saved_y[widget->child_count];
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        saved_y[i] = child->bounds.y;
        child->bounds.y -= page->scroll_position;
    }
    
    // Render children
    for (size_t i = 0; i < widget->child_count; i++) {
        PkError err = widget_render(widget->children[i], renderer);
        if (err != PK_OK) {
            // Restore original positions before returning
            for (size_t j = 0; j < widget->child_count; j++) {
                widget->children[j]->bounds.y = saved_y[j];
            }
            SDL_RenderSetClipRect(renderer, NULL);
            return err;
        }
    }
    
    // Restore original positions
    for (size_t i = 0; i < widget->child_count; i++) {
        widget->children[i]->bounds.y = saved_y[i];
    }
    
    // Clear clip rect
    SDL_RenderSetClipRect(renderer, NULL);
    
    // Draw scroll indicator if needed
    if (page->max_scroll > 0) {
        SDL_Rect scroll_track = {
            widget->bounds.x + widget->bounds.w - 10,
            widget->bounds.y + 50,
            8,
            widget->bounds.h - 50
        };
        
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 100);
        SDL_RenderFillRect(renderer, &scroll_track);
        
        // Scroll thumb
        float scroll_ratio = (float)page->scroll_position / page->max_scroll;
        float thumb_height = (float)(widget->bounds.h - 50) * 
                           ((float)(widget->bounds.h - 50) / page->content_height);
        if (thumb_height < 20) thumb_height = 20;
        
        SDL_Rect scroll_thumb = {
            scroll_track.x + 1,
            scroll_track.y + (int)(scroll_ratio * (scroll_track.h - thumb_height)),
            6,
            (int)thumb_height
        };
        
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 200);
        if (SDL_RenderFillRect(renderer, &scroll_thumb) < 0) {
            pk_set_last_error_with_context(PK_ERROR_RENDER_FAILED,
                                           "Failed to draw scroll thumb: %s",
                                           SDL_GetError());
            return PK_ERROR_RENDER_FAILED;
        }
    }
    
    return PK_OK;
}

static void page_widget_handle_event(Widget* widget, const SDL_Event* event) {
    PageWidget* page = (PageWidget*)widget;
    if (!page) {
        return;
    }
    
    // Handle scroll events
    if (event->type == SDL_MOUSEWHEEL && 
        widget_contains_point(widget, event->wheel.mouseX, event->wheel.mouseY)) {
        page_widget_scroll(page, -event->wheel.y * 20);
    }
    
    // Handle touch/mouse drag for scrolling
    static bool is_dragging = false;
    static int drag_start_y = 0;
    static int drag_start_scroll = 0;
    
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (widget_contains_point(widget, event->button.x, event->button.y)) {
                is_dragging = true;
                drag_start_y = event->button.y;
                drag_start_scroll = page->scroll_position;
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (is_dragging) {
                int delta = drag_start_y - event->motion.y;
                page_widget_set_scroll_position(page, drag_start_scroll + delta);
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            is_dragging = false;
            break;
    }
    
    // Let children handle events (with scroll offset)
    for (size_t i = 0; i < widget->child_count; i++) {
        widget_handle_event(widget->children[i], event);
    }
}

static void page_widget_layout(Widget* widget) {
    PageWidget* page = (PageWidget*)widget;
    if (!page) {
        return;
    }
    
    // Update children positions relative to page
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        
        // Update absolute position based on page position
        child->bounds.x = widget->bounds.x + child->relative_bounds.x;
        child->bounds.y = widget->bounds.y + child->relative_bounds.y;
        
        // Perform child layout
        if (child->needs_layout) {
            widget_perform_layout(child);
        }
    }
    
    // Update content height after layout
    page_widget_update_content_height(page);
}

static void page_widget_destroy(Widget* widget) {
    PageWidget* page = (PageWidget*)widget;
    if (!page) {
        return;
    }
    
    // No additional cleanup needed
}