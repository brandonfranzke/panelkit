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

// Create a new time widget
Widget* time_widget_create(const char* id, const char* format, TTF_Font* font);

// Time widget specific functions
void time_widget_set_format(Widget* widget, const char* format);
void time_widget_set_show_seconds(Widget* widget, bool show_seconds);

#endif // TIME_WIDGET_H