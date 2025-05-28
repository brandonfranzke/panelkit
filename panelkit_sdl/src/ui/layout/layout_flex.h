/**
 * @file layout_flex.h
 * @brief Flexbox-style layout engine for PanelKit
 * 
 * Implements a simplified flexbox layout model for arranging widgets in
 * rows or columns with flexible sizing and spacing.
 */

#ifndef PANELKIT_LAYOUT_FLEX_H
#define PANELKIT_LAYOUT_FLEX_H

#include "layout_core.h"

/** Flex direction for main axis */
typedef enum {
    FLEX_DIRECTION_ROW,            /**< Horizontal layout (left to right) */
    FLEX_DIRECTION_ROW_REVERSE,    /**< Horizontal layout (right to left) */
    FLEX_DIRECTION_COLUMN,         /**< Vertical layout (top to bottom) */
    FLEX_DIRECTION_COLUMN_REVERSE  /**< Vertical layout (bottom to top) */
} FlexDirection;

/** Alignment along the main axis */
typedef enum {
    FLEX_JUSTIFY_START,            /**< Pack items at start */
    FLEX_JUSTIFY_END,              /**< Pack items at end */
    FLEX_JUSTIFY_CENTER,           /**< Center items */
    FLEX_JUSTIFY_SPACE_BETWEEN,    /**< Even space between items */
    FLEX_JUSTIFY_SPACE_AROUND,     /**< Even space around items */
    FLEX_JUSTIFY_SPACE_EVENLY      /**< Even space between and around */
} FlexJustify;

/** Alignment along the cross axis */
typedef enum {
    FLEX_ALIGN_START,          /**< Align to start of cross axis */
    FLEX_ALIGN_END,            /**< Align to end of cross axis */
    FLEX_ALIGN_CENTER,         /**< Center on cross axis */
    FLEX_ALIGN_STRETCH         /**< Stretch to fill cross axis */
} FlexAlign;

/** Wrapping behavior */
typedef enum {
    FLEX_WRAP_NONE,            /**< Single line, items may overflow */
    FLEX_WRAP_WRAP,            /**< Wrap to multiple lines */
    FLEX_WRAP_REVERSE          /**< Wrap with reversed cross axis */
} FlexWrap;

/** Per-widget flex properties */
typedef struct {
    Widget* widget;            /**< Widget this applies to */
    float grow;                /**< Flex grow factor (0 = no grow) */
    float shrink;              /**< Flex shrink factor (0 = no shrink) */
    float basis;               /**< Base size before flex calculations */
    FlexAlign align_self;      /**< Override alignment for this item */
} FlexChildProp;

/** Flex layout configuration */
struct FlexLayoutData {
    // Container properties
    FlexDirection direction;   /**< Main axis direction */
    FlexJustify justify;       /**< Main axis alignment */
    FlexAlign align_items;     /**< Cross axis alignment */
    FlexAlign align_content;   /**< Multi-line cross axis alignment */
    FlexWrap wrap;            /**< Wrapping behavior */
    float line_gap;           /**< Gap between wrapped lines */
    
    // Child properties array
    FlexChildProp* child_props;   /**< Array of child properties */
    size_t child_count;           /**< Number of child properties */
    size_t child_capacity;        /**< Allocated capacity */
};

/**
 * Create a flex layout specification.
 * 
 * @param direction Main axis direction
 * @return New layout spec or NULL on error
 * @note Caller owns the returned spec and must call layout_spec_destroy()
 */
LayoutSpec* layout_flex_create(FlexDirection direction);

/**
 * Set flex container properties.
 * 
 * @param spec Layout specification
 * @param justify Main axis alignment
 * @param align_items Cross axis alignment
 * @return PK_OK on success, error code on failure
 */
PkError layout_flex_set_container(LayoutSpec* spec, FlexJustify justify, 
                                  FlexAlign align_items);

/**
 * Set gap between flex items.
 * 
 * @param spec Layout specification  
 * @param line_gap Space between wrapped lines
 * @return PK_OK on success, error code on failure
 * @note Use layout_spec_set_gap() for gap between items
 */
PkError layout_flex_set_line_gap(LayoutSpec* spec, float line_gap);

/**
 * Set flex properties for a child widget.
 * 
 * @param spec Layout specification
 * @param widget Widget to configure
 * @param grow Flex grow factor
 * @param shrink Flex shrink factor  
 * @param basis Base size (0 for auto)
 * @return PK_OK on success, error code on failure
 */
PkError layout_flex_set_child(LayoutSpec* spec, Widget* widget, 
                             float grow, float shrink, float basis);

/**
 * Get the flex layout engine.
 * 
 * @return Pointer to the flex layout engine
 * @note The returned engine is statically allocated, do not free
 */
LayoutEngine* layout_flex_get_engine(void);

/**
 * Destroy flex layout data.
 * 
 * @param data Data to destroy (can be NULL)
 * @note This is called by layout_spec_destroy, not directly by users
 */
void layout_flex_data_destroy(FlexLayoutData* data);

#endif // PANELKIT_LAYOUT_FLEX_H