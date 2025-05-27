/**
 * @file data_display_widget.h
 * @brief Widget for displaying user data
 * 
 * Specialized widget for showing formatted user information
 * retrieved from API calls.
 */

#ifndef DATA_DISPLAY_WIDGET_H
#define DATA_DISPLAY_WIDGET_H

#include "../widget.h"
#include "text_widget.h"

typedef struct DataDisplayWidget {
    Widget base;
    
    // Text widgets for different fields
    TextWidget* name_widget;
    TextWidget* email_widget;
    TextWidget* phone_widget;
    TextWidget* location_widget;
    
    // Fonts
    TTF_Font* label_font;
    TTF_Font* value_font;
    
    // Layout
    int line_spacing;
    int label_width;
} DataDisplayWidget;

/**
 * Create a new data display widget for showing user information.
 * 
 * @param id Unique identifier for the widget
 * @param label_font Font for field labels (borrowed reference)
 * @param value_font Font for field values (borrowed reference)
 * @return New data display widget or NULL on error (caller owns)
 */
Widget* data_display_widget_create(const char* id, TTF_Font* label_font, TTF_Font* value_font);

/**
 * Set user data to display.
 * 
 * @param widget Data display widget
 * @param name User's full name (copied)
 * @param email User's email address (copied)
 * @param phone User's phone number (copied)
 * @param city User's city (copied)
 * @param country User's country (copied)
 * @note Pass NULL for any field to leave it empty
 */
void data_display_widget_set_user_data(Widget* widget, const char* name, 
                                      const char* email, const char* phone, 
                                      const char* city, const char* country);

/**
 * Clear all displayed data.
 * 
 * @param widget Data display widget to clear
 */
void data_display_widget_clear(Widget* widget);

#endif // DATA_DISPLAY_WIDGET_H