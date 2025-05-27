#ifndef TIME_WIDGET_H
#define TIME_WIDGET_H

#include "../widget.h"
#include "text_widget.h"
#include <time.h>

typedef struct TimeWidget {
    Widget base;
    
    TextWidget* text_widget;  // Internal text widget for display
    char time_format[32];     // strftime format string
    time_t last_update;       // Last update timestamp
    bool show_seconds;
} TimeWidget;

/**
 * Create a new time display widget.
 * 
 * @param id Unique identifier for the widget
 * @param format strftime format string (e.g., "%H:%M:%S")
 * @param font Font to use for time display (borrowed reference)
 * @return New time widget or NULL on error (caller owns)
 * @note Widget automatically updates every second
 */
Widget* time_widget_create(const char* id, const char* format, TTF_Font* font);

/**
 * Set the time display format.
 * 
 * @param widget Time widget to update
 * @param format New strftime format string (copied)
 */
void time_widget_set_format(Widget* widget, const char* format);

/**
 * Enable or disable seconds display.
 * 
 * @param widget Time widget to update
 * @param show_seconds True to update every second, false for every minute
 * @note Affects update frequency and battery usage
 */
void time_widget_set_show_seconds(Widget* widget, bool show_seconds);

#endif // TIME_WIDGET_H