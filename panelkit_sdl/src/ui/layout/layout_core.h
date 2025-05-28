/**
 * @file layout_core.h
 * @brief Core layout system definitions and interfaces
 * 
 * Provides the foundation for widget layout calculations including
 * absolute positioning, flexbox, and grid layouts.
 */

#ifndef LAYOUT_CORE_H
#define LAYOUT_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include "core/error.h"

// Forward declarations
typedef struct Widget Widget;
typedef struct AbsoluteLayoutData AbsoluteLayoutData;
typedef struct FlexLayoutData FlexLayoutData;
typedef struct GridLayoutData GridLayoutData;

/**
 * Layout rectangle using floating point coordinates.
 * Can represent both absolute pixels and relative values (0.0-1.0).
 */
typedef struct LayoutRect {
    float x;         /**< X position (left edge) */
    float y;         /**< Y position (top edge) */
    float width;     /**< Width */
    float height;    /**< Height */
} LayoutRect;

/**
 * Layout type enumeration.
 */
typedef enum {
    LAYOUT_TYPE_NONE = 0,     /**< No layout (widget manages own position) */
    LAYOUT_TYPE_ABSOLUTE,     /**< Absolute positioning */
    LAYOUT_TYPE_FLEX,         /**< Flexbox-style layout */
    LAYOUT_TYPE_GRID          /**< Grid-based layout */
} LayoutType;

/**
 * Display rotation for hardware mounting.
 */
typedef enum {
    DISPLAY_ROTATION_0 = 0,   /**< Normal orientation */
    DISPLAY_ROTATION_90,      /**< Rotated 90 degrees clockwise */
    DISPLAY_ROTATION_180,     /**< Rotated 180 degrees */
    DISPLAY_ROTATION_270      /**< Rotated 270 degrees clockwise */
} DisplayRotation;

/**
 * Display transformation for hardware mounting adjustments.
 */
typedef struct DisplayTransform {
    DisplayRotation rotation;  /**< Rotation angle */
    bool flip_horizontal;      /**< Mirror across vertical axis */
    bool flip_vertical;        /**< Mirror across horizontal axis */
} DisplayTransform;

/**
 * Layout specification - defines how children should be arranged.
 */
typedef struct LayoutSpec {
    LayoutType type;           /**< Type of layout */
    
    // Type-specific data using union for type safety
    union {
        AbsoluteLayoutData* absolute;  /**< Absolute layout configuration */
        FlexLayoutData* flex;          /**< Flexbox layout configuration */
        GridLayoutData* grid;          /**< Grid layout configuration */
    } data;
    
    // Common properties
    float padding_top;         /**< Padding inside container */
    float padding_right;
    float padding_bottom;
    float padding_left;
    
    float gap;                 /**< Gap between children */
    
    // Overflow handling
    bool clip_overflow;        /**< Clip children that exceed bounds (default: true) */
} LayoutSpec;

/**
 * Layout result for a single widget.
 */
typedef struct LayoutResult {
    LayoutRect computed_rect;  /**< Final computed position/size */
    bool visible;              /**< Whether widget is visible */
    bool clipped;              /**< Whether widget was clipped */
} LayoutResult;

/**
 * Layout context for calculations.
 */
typedef struct LayoutContext {
    LayoutRect available_rect;     /**< Available space from parent */
    DisplayTransform* transform;   /**< Display transformation (optional) */
    float scale_factor;            /**< DPI scale factor */
    
    // For percentage calculations
    float reference_width;         /**< Reference width for percentages */
    float reference_height;        /**< Reference height for percentages */
} LayoutContext;

/**
 * Layout engine interface.
 */
typedef struct LayoutEngine {
    /**
     * Calculate layout for a widget and its children.
     * 
     * @param widget Widget to layout
     * @param spec Layout specification
     * @param context Layout context
     * @param results Array to store results (must be large enough for all children)
     * @return Error code
     */
    PkError (*calculate)(const Widget* widget, const LayoutSpec* spec,
                        const LayoutContext* context, LayoutResult* results);
    
    /**
     * Get minimum size requirements.
     * 
     * @param widget Widget to measure
     * @param spec Layout specification
     * @param min_width Output for minimum width
     * @param min_height Output for minimum height
     * @return Error code
     */
    PkError (*get_min_size)(const Widget* widget, const LayoutSpec* spec,
                           float* min_width, float* min_height);
    
    /**
     * Destroy layout engine.
     * 
     * @param engine Engine to destroy
     */
    void (*destroy)(struct LayoutEngine* engine);
} LayoutEngine;

/* Layout specification management */

/**
 * Create a layout specification.
 * 
 * @param type Layout type
 * @return New layout spec or NULL on error
 */
LayoutSpec* layout_spec_create(LayoutType type);

/**
 * Set padding for all sides.
 * 
 * @param spec Layout specification
 * @param padding Padding value for all sides
 */
void layout_spec_set_padding(LayoutSpec* spec, float padding);

/**
 * Set individual padding values.
 * 
 * @param spec Layout specification
 * @param top Top padding
 * @param right Right padding
 * @param bottom Bottom padding
 * @param left Left padding
 */
void layout_spec_set_padding_individual(LayoutSpec* spec, float top, 
                                       float right, float bottom, float left);

/**
 * Set gap between children.
 * 
 * @param spec Layout specification
 * @param gap Gap size
 */
void layout_spec_set_gap(LayoutSpec* spec, float gap);

/**
 * Destroy a layout specification.
 * 
 * @param spec Specification to destroy
 */
void layout_spec_destroy(LayoutSpec* spec);

/* Layout calculation */

/**
 * Calculate layout for a widget tree.
 * 
 * @param widget Root widget
 * @param spec Layout specification
 * @param context Layout context
 * @param results Array for results (must be sized for widget + all descendants)
 * @return Error code
 */
PkError layout_calculate(const Widget* widget, const LayoutSpec* spec,
                        const LayoutContext* context, LayoutResult* results);

/* Coordinate transformation */

/**
 * Transform logical coordinates to physical display coordinates.
 * 
 * @param logical Logical rectangle
 * @param transform Display transformation
 * @param display_width Physical display width
 * @param display_height Physical display height
 * @return Transformed rectangle
 */
LayoutRect layout_transform_to_display(const LayoutRect* logical,
                                      const DisplayTransform* transform,
                                      float display_width, float display_height);

/**
 * Transform physical display coordinates to logical coordinates.
 * 
 * @param physical Physical rectangle
 * @param transform Display transformation
 * @param display_width Physical display width
 * @param display_height Physical display height
 * @return Logical rectangle
 */
LayoutRect layout_transform_from_display(const LayoutRect* physical,
                                        const DisplayTransform* transform,
                                        float display_width, float display_height);

/* Utility functions */

/**
 * Convert layout rectangle to pixel coordinates.
 * 
 * @param layout Layout rectangle (may use relative coordinates)
 * @param parent_width Parent width in pixels
 * @param parent_height Parent height in pixels
 * @return Rectangle in pixels
 */
LayoutRect layout_to_pixels(const LayoutRect* layout,
                           float parent_width, float parent_height);

/**
 * Check if a point is inside a rectangle.
 * 
 * @param rect Rectangle to test
 * @param x X coordinate
 * @param y Y coordinate
 * @return True if point is inside
 */
bool layout_rect_contains_point(const LayoutRect* rect, float x, float y);

/**
 * Calculate intersection of two rectangles.
 * 
 * @param a First rectangle
 * @param b Second rectangle
 * @param result Output for intersection (can be NULL to just test)
 * @return True if rectangles intersect
 */
bool layout_rect_intersect(const LayoutRect* a, const LayoutRect* b,
                          LayoutRect* result);

#endif // LAYOUT_CORE_H