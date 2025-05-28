/**
 * @file ui_init.h
 * @brief Temporary UI initialization - will be replaced by layout/theme system
 */

#ifndef UI_INIT_H
#define UI_INIT_H

#include "widget_manager.h"
#include "../events/event_system.h"
#include "../state/state_store.h"
#include "../core/sdl_includes.h"
#include "core/error.h"

/**
 * Initialize the UI system with hardcoded layout
 * 
 * @param widget_manager Widget manager instance (required)
 * @param event_system Event system for widget communication (required)
 * @param state_store State store for widget data (required)  
 * @param screen_width Screen width in pixels
 * @param screen_height Screen height in pixels
 * @param font_regular Regular font (temporary until theme system)
 * @param font_large Large font (temporary until theme system)
 * @param font_small Small font (temporary until theme system)
 * @return PK_OK on success, error code on failure
 */
PkError ui_init(WidgetManager* widget_manager, 
                EventSystem* event_system,
                StateStore* state_store,
                int screen_width, int screen_height,
                TTF_Font* font_regular,
                TTF_Font* font_large,
                TTF_Font* font_small);

/**
 * Create widgets for all pages (internal helper)
 */
void ui_create_page_widgets(void);

/**
 * Update UI rendering based on current state
 */
void ui_update_rendering(void);

/**
 * Get the page manager widget
 * @return Page manager widget or NULL if not initialized
 */
Widget* ui_get_page_manager(void);

/**
 * Cleanup UI system
 */
void ui_cleanup(void);

// Temporary global that was in app.c
extern bool g_show_time;

#endif // UI_INIT_H