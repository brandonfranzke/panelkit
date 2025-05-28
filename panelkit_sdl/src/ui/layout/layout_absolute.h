/**
 * @file layout_absolute.h
 * @brief Absolute positioning layout engine
 * 
 * Provides direct x,y,width,height positioning for widgets.
 * This is the simplest layout type, similar to traditional UI positioning.
 */

#ifndef PANELKIT_LAYOUT_ABSOLUTE_H
#define PANELKIT_LAYOUT_ABSOLUTE_H

#include "layout_core.h"

/**
 * Absolute layout configuration data.
 * Currently empty as absolute layout has no special configuration,
 * but defined for consistency and future expansion.
 */
struct AbsoluteLayoutData {
    // Reserved for future use (e.g., anchor points, constraints)
    int reserved;
};

/**
 * Create an absolute layout specification.
 * 
 * @return New layout spec or NULL on error
 * @note Caller owns the returned spec and must call layout_spec_destroy()
 */
LayoutSpec* layout_absolute_create(void);

/**
 * Set absolute bounds for a widget.
 * 
 * @param widget Widget to position
 * @param x X position (>1.0 for pixels, 0.0-1.0 for relative)
 * @param y Y position (>1.0 for pixels, 0.0-1.0 for relative)
 * @param width Width (>1.0 for pixels, 0.0-1.0 for relative)
 * @param height Height (>1.0 for pixels, 0.0-1.0 for relative)
 * @return PK_OK on success, error code on failure
 */
PkError layout_absolute_set_bounds(Widget* widget, float x, float y, 
                                  float width, float height);

/**
 * Get the absolute layout engine.
 * 
 * @return Pointer to the absolute layout engine
 * @note The returned engine is statically allocated, do not free
 */
LayoutEngine* layout_absolute_get_engine(void);

/**
 * Destroy absolute layout data.
 * 
 * @param data Data to destroy (can be NULL)
 * @note This is called by layout_spec_destroy, not directly by users
 */
void layout_absolute_data_destroy(AbsoluteLayoutData* data);

#endif // PANELKIT_LAYOUT_ABSOLUTE_H