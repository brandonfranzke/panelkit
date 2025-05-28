/**
 * @file layout_core.c
 * @brief Core layout system implementation
 */

#include "layout_core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Utility functions implementation */

bool layout_rect_contains_point(const LayoutRect* rect, float x, float y) {
    if (!rect) return false;
    
    return x >= rect->x && 
           x < (rect->x + rect->width) &&
           y >= rect->y && 
           y < (rect->y + rect->height);
}

bool layout_rect_intersect(const LayoutRect* a, const LayoutRect* b, LayoutRect* result) {
    if (!a || !b) return false;
    
    // Calculate intersection bounds
    float left = fmaxf(a->x, b->x);
    float top = fmaxf(a->y, b->y);
    float right = fminf(a->x + a->width, b->x + b->width);
    float bottom = fminf(a->y + a->height, b->y + b->height);
    
    // Check if there's an intersection
    if (left < right && top < bottom) {
        if (result) {
            result->x = left;
            result->y = top;
            result->width = right - left;
            result->height = bottom - top;
        }
        return true;
    }
    
    return false;
}

LayoutRect layout_to_pixels(const LayoutRect* layout, float parent_width, float parent_height) {
    if (!layout) {
        return (LayoutRect){0, 0, 0, 0};
    }
    
    LayoutRect result;
    
    // For values <= 1.0, treat as relative (percentage)
    // For values > 1.0, treat as absolute pixels
    
    if (layout->x <= 1.0f) {
        result.x = layout->x * parent_width;
    } else {
        result.x = layout->x;
    }
    
    if (layout->y <= 1.0f) {
        result.y = layout->y * parent_height;
    } else {
        result.y = layout->y;
    }
    
    if (layout->width <= 1.0f) {
        result.width = layout->width * parent_width;
    } else {
        result.width = layout->width;
    }
    
    if (layout->height <= 1.0f) {
        result.height = layout->height * parent_height;
    } else {
        result.height = layout->height;
    }
    
    return result;
}

LayoutRect layout_transform_to_display(const LayoutRect* logical,
                                      const DisplayTransform* transform,
                                      float display_width, float display_height) {
    if (!logical || !transform) {
        return *logical;
    }
    
    LayoutRect result = *logical;
    
    // Apply rotation
    switch (transform->rotation) {
        case DISPLAY_ROTATION_0:
            // No change
            break;
            
        case DISPLAY_ROTATION_90:
            // 90° clockwise: (x,y) -> (h-y-height, x)
            result.x = display_height - logical->y - logical->height;
            result.y = logical->x;
            result.width = logical->height;
            result.height = logical->width;
            break;
            
        case DISPLAY_ROTATION_180:
            // 180°: (x,y) -> (w-x-width, h-y-height)
            result.x = display_width - logical->x - logical->width;
            result.y = display_height - logical->y - logical->height;
            break;
            
        case DISPLAY_ROTATION_270:
            // 270° clockwise: (x,y) -> (y, w-x-width)
            result.x = logical->y;
            result.y = display_width - logical->x - logical->width;
            result.width = logical->height;
            result.height = logical->width;
            break;
    }
    
    // Apply flips after rotation
    if (transform->flip_horizontal) {
        result.x = display_width - result.x - result.width;
    }
    
    if (transform->flip_vertical) {
        result.y = display_height - result.y - result.height;
    }
    
    return result;
}

LayoutRect layout_transform_from_display(const LayoutRect* physical,
                                        const DisplayTransform* transform,
                                        float display_width, float display_height) {
    if (!physical || !transform) {
        return *physical;
    }
    
    // Reverse transformation - apply flips first, then reverse rotation
    LayoutRect result = *physical;
    
    // Reverse flips
    if (transform->flip_horizontal) {
        result.x = display_width - result.x - result.width;
    }
    
    if (transform->flip_vertical) {
        result.y = display_height - result.y - result.height;
    }
    
    // Reverse rotation
    switch (transform->rotation) {
        case DISPLAY_ROTATION_0:
            // No change
            break;
            
        case DISPLAY_ROTATION_90:
            // Reverse 90°: (x,y) -> (y, w-x-width)
            {
                float new_x = result.y;
                float new_y = display_height - result.x - result.width;
                result.x = new_x;
                result.y = new_y;
                result.width = physical->height;
                result.height = physical->width;
            }
            break;
            
        case DISPLAY_ROTATION_180:
            // Reverse 180°: same as forward
            result.x = display_width - result.x - result.width;
            result.y = display_height - result.y - result.height;
            break;
            
        case DISPLAY_ROTATION_270:
            // Reverse 270°: (x,y) -> (h-y-height, x)
            {
                float new_x = display_width - result.y - result.height;
                float new_y = result.x;
                result.x = new_x;
                result.y = new_y;
                result.width = physical->height;
                result.height = physical->width;
            }
            break;
    }
    
    return result;
}

/* Layout specification management */

LayoutSpec* layout_spec_create(LayoutType type) {
    LayoutSpec* spec = calloc(1, sizeof(LayoutSpec));
    if (!spec) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate layout specification");
        return NULL;
    }
    
    spec->type = type;
    spec->clip_overflow = true;  // Default to clipping
    
    return spec;
}

void layout_spec_set_padding(LayoutSpec* spec, float padding) {
    if (!spec) return;
    
    spec->padding_top = padding;
    spec->padding_right = padding;
    spec->padding_bottom = padding;
    spec->padding_left = padding;
}

void layout_spec_set_padding_individual(LayoutSpec* spec, float top, 
                                       float right, float bottom, float left) {
    if (!spec) return;
    
    spec->padding_top = top;
    spec->padding_right = right;
    spec->padding_bottom = bottom;
    spec->padding_left = left;
}

void layout_spec_set_gap(LayoutSpec* spec, float gap) {
    if (!spec) return;
    spec->gap = gap;
}

void layout_spec_destroy(LayoutSpec* spec) {
    if (!spec) return;
    
    // Free type-specific data if allocated
    if (spec->type_data) {
        free(spec->type_data);
    }
    
    free(spec);
}