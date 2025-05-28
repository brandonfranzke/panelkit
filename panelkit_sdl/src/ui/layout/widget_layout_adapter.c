/**
 * @file widget_layout_adapter.c
 * @brief Adapter between layout system and widget system implementation
 */

#include "widget_layout_adapter.h"
#include "core/logger.h"
#include <math.h>

LayoutRect widget_get_layout_bounds(const Widget* widget) {
    if (!widget) {
        return (LayoutRect){0, 0, 0, 0};
    }
    
    // Check if relative bounds are encoded as permille (values > 100 indicate permille encoding)
    if (widget->parent && widget->relative_bounds.w >= 100) {
        // Decode from permille back to float
        return (LayoutRect){
            .x = widget->relative_bounds.x / 1000.0f,
            .y = widget->relative_bounds.y / 1000.0f,
            .width = widget->relative_bounds.w / 1000.0f,
            .height = widget->relative_bounds.h / 1000.0f
        };
    }
    
    // Otherwise use bounds as-is
    SDL_Rect bounds = widget->parent ? widget->relative_bounds : widget->bounds;
    
    return (LayoutRect){
        .x = (float)bounds.x,
        .y = (float)bounds.y,
        .width = (float)bounds.w,
        .height = (float)bounds.h
    };
}

void widget_set_layout_bounds(Widget* widget, const LayoutRect* layout_rect) {
    if (!widget || !layout_rect) return;
    
    // Convert LayoutRect to SDL_Rect (with rounding)
    widget->bounds.x = (int)roundf(layout_rect->x);
    widget->bounds.y = (int)roundf(layout_rect->y);
    widget->bounds.w = (int)roundf(layout_rect->width);
    widget->bounds.h = (int)roundf(layout_rect->height);
    
    // Update relative bounds if widget has parent
    if (widget->parent) {
        widget->relative_bounds.x = widget->bounds.x - widget->parent->bounds.x;
        widget->relative_bounds.y = widget->bounds.y - widget->parent->bounds.y;
        widget->relative_bounds.w = widget->bounds.w;
        widget->relative_bounds.h = widget->bounds.h;
    } else {
        widget->relative_bounds = widget->bounds;
    }
    
    // Mark widget as needing layout update
    widget->needs_layout = true;
}

size_t widget_count_tree(const Widget* widget) {
    if (!widget) return 0;
    
    size_t count = 1; // Count self
    
    for (size_t i = 0; i < widget->child_count; i++) {
        count += widget_count_tree(widget->children[i]);
    }
    
    return count;
}

// Helper to apply results recursively
static PkError apply_results_recursive(Widget* widget, const LayoutResult* results,
                                      size_t result_count, size_t* index) {
    if (!widget || !results || !index) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in apply_results_recursive");
        return PK_ERROR_NULL_PARAM;
    }
    
    if (*index >= result_count) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Not enough layout results for widget tree");
        return PK_ERROR_INVALID_PARAM;
    }
    
    // Apply result to current widget
    const LayoutResult* result = &results[(*index)++];
    widget_set_layout_bounds(widget, &result->computed_rect);
    
    // Update visibility
    if (!result->visible) {
        widget->state_flags |= WIDGET_STATE_HIDDEN;
    } else {
        widget->state_flags &= ~WIDGET_STATE_HIDDEN;
    }
    
    // Mark dirty if clipped (for render optimization)
    if (result->clipped) {
        widget->state_flags |= WIDGET_STATE_DIRTY;
        log_debug("Widget '%s' marked dirty due to clipping", widget->id);
    }
    
    // Apply to children
    for (size_t i = 0; i < widget->child_count; i++) {
        PkError err = apply_results_recursive(widget->children[i], results, 
                                             result_count, index);
        if (err != PK_OK) {
            return err;
        }
    }
    
    return PK_OK;
}

PkError widget_apply_layout_results(Widget* widget, const LayoutResult* results,
                                   size_t result_count) {
    if (!widget || !results) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in widget_apply_layout_results");
        return PK_ERROR_NULL_PARAM;
    }
    
    size_t index = 0;
    PkError err = apply_results_recursive(widget, results, result_count, &index);
    
    if (err == PK_OK && index != result_count) {
        log_warn("Layout result count mismatch: used %zu of %zu results", 
                    index, result_count);
    }
    
    return err;
}