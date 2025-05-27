/**
 * @file page_widget.h
 * @brief Page container widget
 * 
 * Container widget that represents a full-screen page in the UI.
 */

#ifndef PAGE_WIDGET_H
#define PAGE_WIDGET_H

#include "widget.h"

// Page widget - base class for all pages in the application
typedef struct PageWidget {
    Widget base;  // Must be first member for casting
    
    // Page properties
    char title[128];
    int page_index;
    
    // Scroll management
    int scroll_position;
    int max_scroll;
    int content_height;
    
    // Page-specific styling
    SDL_Color background_color;
    SDL_Color title_color;
    
    // Optional callbacks
    void (*on_enter)(struct PageWidget* page);
    void (*on_leave)(struct PageWidget* page);
    void (*on_scroll)(struct PageWidget* page, int delta);
} PageWidget;

// Constructor
PageWidget* page_widget_create(const char* id, const char* title);

// Configuration
void page_widget_set_title(PageWidget* page, const char* title);
void page_widget_set_colors(PageWidget* page, SDL_Color background, SDL_Color title_color);

// Scroll management
void page_widget_scroll(PageWidget* page, int delta);
void page_widget_set_scroll_position(PageWidget* page, int position);
int page_widget_get_scroll_position(PageWidget* page);
void page_widget_update_content_height(PageWidget* page);

// Lifecycle callbacks
void page_widget_set_enter_callback(PageWidget* page, void (*callback)(PageWidget*));
void page_widget_set_leave_callback(PageWidget* page, void (*callback)(PageWidget*));
void page_widget_set_scroll_callback(PageWidget* page, void (*callback)(PageWidget*, int));

// Page lifecycle
void page_widget_enter(PageWidget* page);
void page_widget_leave(PageWidget* page);

// Helper to add scrollable content container
Widget* page_widget_create_content_container(PageWidget* page);

#endif // PAGE_WIDGET_H