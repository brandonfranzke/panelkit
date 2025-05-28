/**
 * @file layout_absolute.c
 * @brief Absolute positioning layout engine implementation
 */

#include "layout_absolute.h"
#include "widget_layout_adapter.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations
static PkError absolute_calculate(const Widget* widget, const LayoutSpec* spec,
                                 const LayoutContext* context, LayoutResult* results);
static PkError absolute_get_min_size(const Widget* widget, const LayoutSpec* spec,
                                    float* min_width, float* min_height);
static void absolute_destroy(LayoutEngine* engine);

// Static engine instance
static LayoutEngine g_absolute_engine = {
    .calculate = absolute_calculate,
    .get_min_size = absolute_get_min_size,
    .destroy = absolute_destroy
};

LayoutSpec* layout_absolute_create(void) {
    LayoutSpec* spec = layout_spec_create(LAYOUT_TYPE_ABSOLUTE);
    if (!spec) {
        return NULL;
    }
    
    // Allocate absolute layout data
    AbsoluteLayoutData* data = calloc(1, sizeof(AbsoluteLayoutData));
    if (!data) {
        layout_spec_destroy(spec);
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate absolute layout data");
        return NULL;
    }
    
    spec->data.absolute = data;
    return spec;
}

PkError layout_absolute_set_bounds(Widget* widget, float x, float y, 
                                  float width, float height) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "Widget is NULL in layout_absolute_set_bounds");
        return PK_ERROR_NULL_PARAM;
    }
    
    // For relative values (0.0-1.0), store them in relative_bounds
    // For absolute values (>1.0), store them in bounds
    if (x <= 1.0f || y <= 1.0f || width <= 1.0f || height <= 1.0f) {
        // Store as relative bounds to preserve float precision
        widget->relative_bounds.x = (int)(x * 1000); // Store as permille
        widget->relative_bounds.y = (int)(y * 1000);
        widget->relative_bounds.w = (int)(width * 1000);
        widget->relative_bounds.h = (int)(height * 1000);
    } else {
        // Store as absolute bounds
        widget->bounds.x = (int)x;
        widget->bounds.y = (int)y;
        widget->bounds.w = (int)width;
        widget->bounds.h = (int)height;
    }
    
    log_debug("Set absolute bounds for widget '%s': x=%.1f, y=%.1f, w=%.1f, h=%.1f",
              widget->id, x, y, width, height);
    
    return PK_OK;
}

// Calculate layout recursively
static PkError calculate_recursive(const Widget* widget, const LayoutSpec* spec,
                                  const LayoutContext* context, LayoutResult* results,
                                  int* result_index, float parent_x, float parent_y) {
    if (!widget || !results || !result_index) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in calculate_recursive");
        return PK_ERROR_NULL_PARAM;
    }
    
    int current_index = (*result_index)++;
    LayoutResult* result = &results[current_index];
    
    // Get widget bounds
    LayoutRect bounds = widget_get_layout_bounds(widget);
    
    // Convert relative coordinates to pixels if needed
    if (bounds.x <= 1.0f && bounds.x >= 0.0f) {
        bounds.x *= context->reference_width;
    }
    if (bounds.y <= 1.0f && bounds.y >= 0.0f) {
        bounds.y *= context->reference_height;
    }
    if (bounds.width <= 1.0f && bounds.width > 0.0f) {
        bounds.width *= context->reference_width;
    }
    if (bounds.height <= 1.0f && bounds.height > 0.0f) {
        bounds.height *= context->reference_height;
    }
    
    // Apply parent offset (parent position already includes padding)
    bounds.x += parent_x;
    bounds.y += parent_y;
    
    // Store computed bounds
    result->computed_rect = bounds;
    result->visible = !(widget->state_flags & WIDGET_STATE_HIDDEN);
    result->clipped = false;
    
    // Check if widget extends beyond parent bounds (if clipping enabled)
    if (spec->clip_overflow && widget->parent) {
        LayoutRect parent_rect = results[0].computed_rect; // Parent is always index 0
        float right = bounds.x + bounds.width;
        float bottom = bounds.y + bounds.height;
        float parent_right = parent_rect.x + parent_rect.width - spec->padding_right;
        float parent_bottom = parent_rect.y + parent_rect.height - spec->padding_bottom;
        
        if (right > parent_right || bottom > parent_bottom ||
            bounds.x < parent_rect.x + spec->padding_left ||
            bounds.y < parent_rect.y + spec->padding_top) {
            result->clipped = true;
            log_debug("Widget '%s' clipped by parent bounds", widget->id);
        }
    }
    
    // Process children
    for (size_t i = 0; i < widget->child_count; i++) {
        // Apply padding offset for children
        float child_offset_x = bounds.x + spec->padding_left;
        float child_offset_y = bounds.y + spec->padding_top;
        
        // Create child context with parent's content area as reference
        LayoutContext child_context = *context;
        child_context.reference_width = bounds.width - spec->padding_left - spec->padding_right;
        child_context.reference_height = bounds.height - spec->padding_top - spec->padding_bottom;
        
        PkError err = calculate_recursive(widget->children[i], spec, &child_context, 
                                         results, result_index, child_offset_x, child_offset_y);
        if (err != PK_OK) {
            return err;
        }
    }
    
    return PK_OK;
}

static PkError absolute_calculate(const Widget* widget, const LayoutSpec* spec,
                                 const LayoutContext* context, LayoutResult* results) {
    if (!widget || !spec || !context || !results) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in absolute_calculate");
        return PK_ERROR_NULL_PARAM;
    }
    
    if (spec->type != LAYOUT_TYPE_ABSOLUTE) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid spec type %d for absolute layout", spec->type);
        return PK_ERROR_INVALID_PARAM;
    }
    
    log_debug("Calculating absolute layout for widget '%s'", widget->id);
    
    int result_index = 0;
    return calculate_recursive(widget, spec, context, results, &result_index, 0, 0);
}

static PkError absolute_get_min_size(const Widget* widget, const LayoutSpec* spec,
                                    float* min_width, float* min_height) {
    if (!widget || !spec || !min_width || !min_height) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in absolute_get_min_size");
        return PK_ERROR_NULL_PARAM;
    }
    
    // For absolute layout, minimum size is the bounds of the furthest child
    float max_right = 0;
    float max_bottom = 0;
    
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child->state_flags & WIDGET_STATE_HIDDEN) {
            continue;
        }
        
        float right = child->bounds.x + child->bounds.w;
        float bottom = child->bounds.y + child->bounds.h;
        
        if (right > max_right) max_right = right;
        if (bottom > max_bottom) max_bottom = bottom;
    }
    
    *min_width = max_right + spec->padding_right;
    *min_height = max_bottom + spec->padding_bottom;
    
    log_debug("Absolute minimum size for '%s': %.1fx%.1f", 
                 widget->id, *min_width, *min_height);
    
    return PK_OK;
}

static void absolute_destroy(LayoutEngine* engine) {
    // Static engine, nothing to destroy
    (void)engine;
}

LayoutEngine* layout_absolute_get_engine(void) {
    return &g_absolute_engine;
}

void layout_absolute_data_destroy(AbsoluteLayoutData* data) {
    if (!data) return;
    // Currently no dynamic data to free
    free(data);
}