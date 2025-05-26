#include "text_widget.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations
static void text_widget_render(Widget* widget, SDL_Renderer* renderer);
static void text_widget_destroy(Widget* widget);
static void text_widget_update_texture(TextWidget* text_widget, SDL_Renderer* renderer);

Widget* text_widget_create(const char* id, const char* text, TTF_Font* font) {
    TextWidget* text_widget = calloc(1, sizeof(TextWidget));
    if (!text_widget) return NULL;
    
    // Initialize base widget
    Widget* base = &text_widget->base;
    strncpy(base->id, id, sizeof(base->id) - 1);
    base->type = WIDGET_TYPE_LABEL;
    base->state_flags = WIDGET_STATE_NORMAL;
    
    // Set widget methods
    base->render = text_widget_render;
    base->destroy = text_widget_destroy;
    
    // Initialize arrays
    base->child_capacity = 0;
    base->children = NULL;
    base->event_capacity = 0;
    base->subscribed_events = NULL;
    
    // Set text properties
    text_widget->text = text ? strdup(text) : strdup("");
    text_widget->font = font;
    text_widget->color = (SDL_Color){255, 255, 255, 255}; // Default white
    text_widget->alignment = TEXT_ALIGN_CENTER;
    text_widget->wrap = false;
    text_widget->needs_update = true;
    
    // Calculate initial size if font available
    if (font && text) {
        int w, h;
        TTF_SizeText(font, text, &w, &h);
        base->bounds.w = w;
        base->bounds.h = h;
    }
    
    return (Widget*)text_widget;
}

void text_widget_set_text(Widget* widget, const char* text) {
    if (!widget || widget->type != WIDGET_TYPE_LABEL) return;
    TextWidget* text_widget = (TextWidget*)widget;
    
    if (text_widget->text) {
        free(text_widget->text);
    }
    text_widget->text = text ? strdup(text) : strdup("");
    text_widget->needs_update = true;
    widget->state_flags |= WIDGET_STATE_DIRTY;
}

void text_widget_set_color(Widget* widget, SDL_Color color) {
    if (!widget || widget->type != WIDGET_TYPE_LABEL) return;
    TextWidget* text_widget = (TextWidget*)widget;
    
    text_widget->color = color;
    text_widget->needs_update = true;
    widget->state_flags |= WIDGET_STATE_DIRTY;
}

void text_widget_set_alignment(Widget* widget, TextAlignment alignment) {
    if (!widget || widget->type != WIDGET_TYPE_LABEL) return;
    TextWidget* text_widget = (TextWidget*)widget;
    
    text_widget->alignment = alignment;
    widget->state_flags |= WIDGET_STATE_DIRTY;
}

void text_widget_set_font(Widget* widget, TTF_Font* font) {
    if (!widget || widget->type != WIDGET_TYPE_LABEL) return;
    TextWidget* text_widget = (TextWidget*)widget;
    
    text_widget->font = font;
    text_widget->needs_update = true;
    widget->state_flags |= WIDGET_STATE_DIRTY;
}

static void text_widget_update_texture(TextWidget* text_widget, SDL_Renderer* renderer) {
    if (!text_widget->needs_update || !text_widget->font || !renderer) return;
    
    // Destroy old texture
    if (text_widget->texture) {
        SDL_DestroyTexture(text_widget->texture);
        text_widget->texture = NULL;
    }
    
    // Create surface from text
    SDL_Surface* surface = TTF_RenderText_Blended(text_widget->font, 
                                                  text_widget->text, 
                                                  text_widget->color);
    if (!surface) return;
    
    // Create texture from surface
    text_widget->texture = SDL_CreateTextureFromSurface(renderer, surface);
    text_widget->texture_width = surface->w;
    text_widget->texture_height = surface->h;
    
    SDL_FreeSurface(surface);
    text_widget->needs_update = false;
}

static void text_widget_render(Widget* widget, SDL_Renderer* renderer) {
    if (!widget || !renderer) return;
    TextWidget* text_widget = (TextWidget*)widget;
    
    // Update texture if needed
    text_widget_update_texture(text_widget, renderer);
    
    if (!text_widget->texture) return;
    
    // Calculate destination rectangle based on alignment
    SDL_Rect dest = widget->bounds;
    
    switch (text_widget->alignment) {
        case TEXT_ALIGN_CENTER:
            dest.x = widget->bounds.x + (widget->bounds.w - text_widget->texture_width) / 2;
            break;
        case TEXT_ALIGN_RIGHT:
            dest.x = widget->bounds.x + widget->bounds.w - text_widget->texture_width;
            break;
        case TEXT_ALIGN_LEFT:
        default:
            dest.x = widget->bounds.x;
            break;
    }
    
    dest.y = widget->bounds.y + (widget->bounds.h - text_widget->texture_height) / 2;
    dest.w = text_widget->texture_width;
    dest.h = text_widget->texture_height;
    
    SDL_RenderCopy(renderer, text_widget->texture, NULL, &dest);
}

static void text_widget_destroy(Widget* widget) {
    if (!widget) return;
    TextWidget* text_widget = (TextWidget*)widget;
    
    if (text_widget->text) {
        free(text_widget->text);
    }
    if (text_widget->texture) {
        SDL_DestroyTexture(text_widget->texture);
    }
}