/**
 * @file text_widget.h
 * @brief Simple text display widget
 * 
 * Widget for displaying static or dynamic text with alignment options.
 */

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

/**
 * Create a new text widget.
 * 
 * @param id Unique identifier for the widget
 * @param text Initial text to display (copied)
 * @param font Font to use for rendering (borrowed reference)
 * @return New text widget or NULL on error (caller owns)
 */
Widget* text_widget_create(const char* id, const char* text, TTF_Font* font);

/**
 * Set the text content of a text widget.
 * 
 * @param widget Text widget to update
 * @param text New text to display (copied)
 * @note Invalidates cached texture
 */
void text_widget_set_text(Widget* widget, const char* text);

/**
 * Set the text color.
 * 
 * @param widget Text widget to update
 * @param color New text color
 */
void text_widget_set_color(Widget* widget, SDL_Color color);

/**
 * Set text alignment within the widget bounds.
 * 
 * @param widget Text widget to update
 * @param alignment Text alignment mode
 */
void text_widget_set_alignment(Widget* widget, TextAlignment alignment);

/**
 * Set the font for text rendering.
 * 
 * @param widget Text widget to update
 * @param font New font to use (borrowed reference)
 * @note Font must remain valid while widget uses it
 */
void text_widget_set_font(Widget* widget, TTF_Font* font);

#endif // TEXT_WIDGET_H