/**
 * @file layout_flex.c
 * @brief Flexbox layout engine implementation
 */

#include "layout_flex.h"
#include "widget_layout_adapter.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Forward declarations
static PkError flex_calculate(const Widget* widget, const LayoutSpec* spec,
                             const LayoutContext* context, LayoutResult* results);
static PkError flex_get_min_size(const Widget* widget, const LayoutSpec* spec,
                                float* min_width, float* min_height);
static void flex_destroy(LayoutEngine* engine);

// Static engine instance
static LayoutEngine g_flex_engine = {
    .calculate = flex_calculate,
    .get_min_size = flex_get_min_size,
    .destroy = flex_destroy
};

// Helper: Check if direction is row-based
static bool is_row_direction(FlexDirection dir) {
    return dir == FLEX_DIRECTION_ROW || dir == FLEX_DIRECTION_ROW_REVERSE;
}

// Helper: Check if direction is reversed
static bool is_reverse_direction(FlexDirection dir) {
    return dir == FLEX_DIRECTION_ROW_REVERSE || dir == FLEX_DIRECTION_COLUMN_REVERSE;
}

// Helper: Find flex properties for a widget
static FlexChildProp* find_child_props(FlexLayoutData* data, const Widget* widget) {
    if (!data || !widget) return NULL;
    
    for (size_t i = 0; i < data->child_count; i++) {
        if (data->child_props[i].widget == widget) {
            return &data->child_props[i];
        }
    }
    
    return NULL;
}

LayoutSpec* layout_flex_create(FlexDirection direction) {
    LayoutSpec* spec = layout_spec_create(LAYOUT_TYPE_FLEX);
    if (!spec) {
        return NULL;
    }
    
    // Allocate flex layout data
    FlexLayoutData* data = calloc(1, sizeof(FlexLayoutData));
    if (!data) {
        layout_spec_destroy(spec);
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate flex layout data");
        return NULL;
    }
    
    // Set defaults
    data->direction = direction;
    data->justify = FLEX_JUSTIFY_START;
    data->align_items = FLEX_ALIGN_STRETCH;
    data->align_content = FLEX_ALIGN_STRETCH;
    data->wrap = FLEX_WRAP_NONE;
    data->line_gap = 0.0f;
    
    spec->data.flex = data;
    return spec;
}

PkError layout_flex_set_container(LayoutSpec* spec, FlexJustify justify, 
                                  FlexAlign align_items) {
    if (!spec || spec->type != LAYOUT_TYPE_FLEX) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid spec for flex container");
        return PK_ERROR_INVALID_PARAM;
    }
    
    FlexLayoutData* data = spec->data.flex;
    if (!data) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_STATE,
                                       "Missing flex layout data");
        return PK_ERROR_INVALID_STATE;
    }
    
    data->justify = justify;
    data->align_items = align_items;
    
    log_debug("Set flex container: justify=%d, align=%d", justify, align_items);
    
    return PK_OK;
}

PkError layout_flex_set_line_gap(LayoutSpec* spec, float line_gap) {
    if (!spec || spec->type != LAYOUT_TYPE_FLEX) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid spec for flex line gap");
        return PK_ERROR_INVALID_PARAM;
    }
    
    FlexLayoutData* data = spec->data.flex;
    if (!data) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_STATE,
                                       "Missing flex layout data");
        return PK_ERROR_INVALID_STATE;
    }
    
    data->line_gap = line_gap;
    return PK_OK;
}

PkError layout_flex_set_child(LayoutSpec* spec, Widget* widget, 
                             float grow, float shrink, float basis) {
    if (!spec || spec->type != LAYOUT_TYPE_FLEX || !widget) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid parameters for flex child");
        return PK_ERROR_INVALID_PARAM;
    }
    
    FlexLayoutData* data = spec->data.flex;
    if (!data) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_STATE,
                                       "Missing flex layout data");
        return PK_ERROR_INVALID_STATE;
    }
    
    // Check if we already have properties for this widget
    FlexChildProp* props = find_child_props(data, widget);
    
    if (!props) {
        // Need to add new entry
        if (data->child_count >= data->child_capacity) {
            size_t new_capacity = data->child_capacity ? data->child_capacity * 2 : 8;
            FlexChildProp* new_props = realloc(data->child_props, 
                                              new_capacity * sizeof(FlexChildProp));
            if (!new_props) {
                pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                               "Failed to grow flex child properties");
                return PK_ERROR_OUT_OF_MEMORY;
            }
            data->child_props = new_props;
            data->child_capacity = new_capacity;
        }
        
        props = &data->child_props[data->child_count++];
        props->widget = widget;
        props->align_self = FLEX_ALIGN_START; // Default
    }
    
    props->grow = grow;
    props->shrink = shrink;
    props->basis = basis;
    
    log_debug("Set flex child '%s': grow=%.1f, shrink=%.1f, basis=%.1f",
                 widget->id, grow, shrink, basis);
    
    return PK_OK;
}

// Calculate flex layout
static PkError flex_calculate(const Widget* widget, const LayoutSpec* spec,
                             const LayoutContext* context, LayoutResult* results) {
    if (!widget || !spec || !context || !results) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in flex_calculate");
        return PK_ERROR_NULL_PARAM;
    }
    
    if (spec->type != LAYOUT_TYPE_FLEX) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid spec type %d for flex layout", spec->type);
        return PK_ERROR_INVALID_PARAM;
    }
    
    FlexLayoutData* flex = spec->data.flex;
    if (!flex) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_STATE,
                                       "Missing flex layout data");
        return PK_ERROR_INVALID_STATE;
    }
    
    log_debug("Calculating flex layout for widget '%s'", widget->id);
    
    // Container bounds
    LayoutRect container = widget_get_layout_bounds(widget);
    
    // Apply padding
    float content_x = container.x + spec->padding_left;
    float content_y = container.y + spec->padding_top;
    float content_width = container.width - spec->padding_left - spec->padding_right;
    float content_height = container.height - spec->padding_top - spec->padding_bottom;
    
    // Convert to pixels if needed
    if (content_width <= 1.0f) content_width *= context->reference_width;
    if (content_height <= 1.0f) content_height *= context->reference_height;
    
    // Store container result
    results[0].computed_rect = (LayoutRect){
        .x = container.x,
        .y = container.y,
        .width = content_width + spec->padding_left + spec->padding_right,
        .height = content_height + spec->padding_top + spec->padding_bottom
    };
    results[0].visible = !(widget->state_flags & WIDGET_STATE_HIDDEN);
    results[0].clipped = false;
    
    // No children? We're done
    if (widget->child_count == 0) {
        return PK_OK;
    }
    
    bool is_row = is_row_direction(flex->direction);
    bool is_reverse = is_reverse_direction(flex->direction);
    
    // Calculate total flex grow/shrink and fixed space
    float total_grow = 0.0f;
    float total_shrink = 0.0f;
    float fixed_space = 0.0f;
    int flex_item_count = 0;
    
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child->state_flags & WIDGET_STATE_HIDDEN) continue;
        
        FlexChildProp* props = find_child_props(flex, child);
        if (!props) {
            // Default properties - use natural size
            fixed_space += is_row ? child->bounds.w : child->bounds.h;
        } else {
            if (props->grow > 0) total_grow += props->grow;
            if (props->shrink > 0) total_shrink += props->shrink;
            
            float basis = props->basis;
            if (basis <= 0) {
                // Auto basis - use widget's natural size
                basis = is_row ? child->bounds.w : child->bounds.h;
            }
            fixed_space += basis;
        }
        flex_item_count++;
    }
    
    // Add gaps
    if (flex_item_count > 1) {
        fixed_space += spec->gap * (flex_item_count - 1);
    }
    
    // Calculate available space
    float available_space = is_row ? content_width : content_height;
    float remaining_space = available_space - fixed_space;
    
    // Determine if we're growing or shrinking
    bool growing = remaining_space > 0;
    float total_flex = growing ? total_grow : total_shrink;
    
    // Calculate starting position based on justification
    float main_pos = 0;
    float extra_gap = 0;
    
    if (total_flex == 0 && remaining_space > 0) {
        // No flexible items, distribute based on justification
        switch (flex->justify) {
            case FLEX_JUSTIFY_END:
                main_pos = remaining_space;
                break;
            case FLEX_JUSTIFY_CENTER:
                main_pos = remaining_space / 2;
                break;
            case FLEX_JUSTIFY_SPACE_BETWEEN:
                if (flex_item_count > 1) {
                    extra_gap = remaining_space / (flex_item_count - 1);
                }
                break;
            case FLEX_JUSTIFY_SPACE_AROUND:
                extra_gap = remaining_space / flex_item_count;
                main_pos = extra_gap / 2;
                break;
            case FLEX_JUSTIFY_SPACE_EVENLY:
                extra_gap = remaining_space / (flex_item_count + 1);
                main_pos = extra_gap;
                break;
            default:
                break;
        }
    }
    
    // Position children
    int result_index = 1; // Start after container
    
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child->state_flags & WIDGET_STATE_HIDDEN) continue;
        
        FlexChildProp* props = find_child_props(flex, child);
        LayoutRect child_bounds = widget_get_layout_bounds(child);
        
        // Calculate main axis size
        float main_size;
        if (props && total_flex > 0 && ((growing && props->grow > 0) || (!growing && props->shrink > 0))) {
            float basis = props->basis > 0 ? props->basis : 
                         (is_row ? child_bounds.width : child_bounds.height);
            float flex_factor = growing ? props->grow : props->shrink;
            float flex_space = (remaining_space * flex_factor) / total_flex;
            main_size = basis + flex_space;
        } else {
            main_size = is_row ? child_bounds.width : child_bounds.height;
        }
        
        // Calculate cross axis size and position
        float cross_size = is_row ? child_bounds.height : child_bounds.width;
        float cross_pos = 0;
        float cross_available = is_row ? content_height : content_width;
        
        FlexAlign align = (props && props->align_self != FLEX_ALIGN_START) ? 
                          props->align_self : flex->align_items;
        
        switch (align) {
            case FLEX_ALIGN_END:
                cross_pos = cross_available - cross_size;
                break;
            case FLEX_ALIGN_CENTER:
                cross_pos = (cross_available - cross_size) / 2;
                break;
            case FLEX_ALIGN_STRETCH:
                cross_size = cross_available;
                break;
            default:
                break;
        }
        
        // Set final position and size
        LayoutResult* result = &results[result_index++];
        
        if (is_row) {
            result->computed_rect.x = content_x + (is_reverse ? 
                                      available_space - main_pos - main_size : main_pos);
            result->computed_rect.y = content_y + cross_pos;
            result->computed_rect.width = main_size;
            result->computed_rect.height = cross_size;
        } else {
            result->computed_rect.x = content_x + cross_pos;
            result->computed_rect.y = content_y + (is_reverse ? 
                                      available_space - main_pos - main_size : main_pos);
            result->computed_rect.width = cross_size;
            result->computed_rect.height = main_size;
        }
        
        result->visible = true;
        result->clipped = spec->clip_overflow && 
                         (result->computed_rect.x < content_x ||
                          result->computed_rect.y < content_y ||
                          result->computed_rect.x + result->computed_rect.width > 
                          content_x + content_width ||
                          result->computed_rect.y + result->computed_rect.height > 
                          content_y + content_height);
        
        if (result->clipped) {
            log_debug("Flex child '%s' clipped", child->id);
        }
        
        // Advance position
        main_pos += main_size + spec->gap + extra_gap;
    }
    
    return PK_OK;
}

static PkError flex_get_min_size(const Widget* widget, const LayoutSpec* spec,
                                float* min_width, float* min_height) {
    if (!widget || !spec || !min_width || !min_height) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL parameter in flex_get_min_size");
        return PK_ERROR_NULL_PARAM;
    }
    
    FlexLayoutData* flex = spec->data.flex;
    if (!flex) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_STATE,
                                       "Missing flex layout data");
        return PK_ERROR_INVALID_STATE;
    }
    
    bool is_row = is_row_direction(flex->direction);
    float main_size = 0;
    float cross_size = 0;
    int visible_count = 0;
    
    // Calculate minimum size based on children
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child->state_flags & WIDGET_STATE_HIDDEN) continue;
        
        float child_width = child->bounds.w;
        float child_height = child->bounds.h;
        
        if (is_row) {
            main_size += child_width;
            if (child_height > cross_size) cross_size = child_height;
        } else {
            main_size += child_height;
            if (child_width > cross_size) cross_size = child_width;
        }
        visible_count++;
    }
    
    // Add gaps
    if (visible_count > 1) {
        main_size += spec->gap * (visible_count - 1);
    }
    
    // Apply padding
    *min_width = (is_row ? main_size : cross_size) + 
                 spec->padding_left + spec->padding_right;
    *min_height = (is_row ? cross_size : main_size) + 
                  spec->padding_top + spec->padding_bottom;
    
    log_debug("Flex minimum size for '%s': %.1fx%.1f", 
                 widget->id, *min_width, *min_height);
    
    return PK_OK;
}

static void flex_destroy(LayoutEngine* engine) {
    // Static engine, nothing to destroy
    (void)engine;
}

LayoutEngine* layout_flex_get_engine(void) {
    return &g_flex_engine;
}

void layout_flex_data_destroy(FlexLayoutData* data) {
    if (!data) return;
    
    if (data->child_props) {
        free(data->child_props);
    }
    
    free(data);
}