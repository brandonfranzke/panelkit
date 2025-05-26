#ifndef TEXT_WIDGET_H
#define TEXT_WIDGET_H

#include "../widget.h"
#include <SDL2/SDL_ttf.h>

typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
} TextAlignment;

typedef struct TextWidget {
    Widget base;
    
    char* text;
    TTF_Font* font;
    SDL_Color color;
    TextAlignment alignment;
    bool wrap;
    int max_width;
    
    // Cached texture for performance
    SDL_Texture* texture;
    int texture_width;
    int texture_height;
    bool needs_update;
} TextWidget;

// Create a new text widget
Widget* text_widget_create(const char* id, const char* text, TTF_Font* font);

// Text widget specific functions
void text_widget_set_text(Widget* widget, const char* text);
void text_widget_set_color(Widget* widget, SDL_Color color);
void text_widget_set_alignment(Widget* widget, TextAlignment alignment);
void text_widget_set_font(Widget* widget, TTF_Font* font);

#endif // TEXT_WIDGET_H