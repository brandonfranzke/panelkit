/**
 * @file widget_layout_adapter.h
 * @brief Adapter between layout system and widget system
 * 
 * Provides functions to bridge between the float-based layout system
 * and the integer-based SDL_Rect widget bounds.
 */

#ifndef PANELKIT_WIDGET_LAYOUT_ADAPTER_H
#define PANELKIT_WIDGET_LAYOUT_ADAPTER_H

#include "layout_core.h"
#include "ui/widget.h"

/**
 * Get layout bounds from a widget.
 * 
 * @param widget Widget to get bounds from
 * @return Layout rectangle with widget bounds
 * @note Uses relative bounds if widget has a parent, absolute bounds otherwise
 */
LayoutRect widget_get_layout_bounds(const Widget* widget);

/**
 * Set widget bounds from layout rectangle.
 * 
 * @param widget Widget to update
 * @param layout_rect Layout rectangle with new bounds
 * @note Converts float coordinates to integer SDL_Rect
 */
void widget_set_layout_bounds(Widget* widget, const LayoutRect* layout_rect);

/**
 * Apply layout results to a widget tree.
 * 
 * @param widget Root widget
 * @param results Array of layout results
 * @param result_count Number of results in array
 * @return PK_OK on success, error code on failure
 * @note Results must be in same order as widget tree traversal
 */
PkError widget_apply_layout_results(Widget* widget, const LayoutResult* results,
                                   size_t result_count);

/**
 * Count total widgets in tree (including root).
 * 
 * @param widget Root widget
 * @return Total number of widgets
 * @note Used to allocate result arrays for layout calculations
 */
size_t widget_count_tree(const Widget* widget);

#endif // PANELKIT_WIDGET_LAYOUT_ADAPTER_H