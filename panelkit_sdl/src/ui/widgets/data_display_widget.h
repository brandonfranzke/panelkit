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

// Create a new data display widget
Widget* data_display_widget_create(const char* id, TTF_Font* label_font, TTF_Font* value_font);

// Data display specific functions
void data_display_widget_set_user_data(Widget* widget, const char* name, 
                                      const char* email, const char* phone, 
                                      const char* city, const char* country);
void data_display_widget_clear(Widget* widget);

#endif // DATA_DISPLAY_WIDGET_H