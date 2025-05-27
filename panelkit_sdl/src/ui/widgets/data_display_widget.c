#include "data_display_widget.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/error.h"

// Forward declarations
static PkError data_display_widget_render(Widget* widget, SDL_Renderer* renderer);
static void data_display_widget_destroy(Widget* widget);
static void data_display_widget_layout(Widget* widget);

Widget* data_display_widget_create(const char* id, TTF_Font* label_font, TTF_Font* value_font) {
    if (!id) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "id is NULL in data_display_widget_create");
        return NULL;
    }
    if (!label_font) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "label_font is NULL in data_display_widget_create");
        return NULL;
    }
    if (!value_font) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "value_font is NULL in data_display_widget_create");
        return NULL;
    }
    
    DataDisplayWidget* data_widget = calloc(1, sizeof(DataDisplayWidget));
    if (!data_widget) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate data display widget");
        return NULL;
    }
    
    // Initialize base widget
    Widget* base = &data_widget->base;
    strncpy(base->id, id, sizeof(base->id) - 1);
    base->type = WIDGET_TYPE_CUSTOM;
    base->state_flags = WIDGET_STATE_NORMAL;
    
    // Set widget methods
    base->render = data_display_widget_render;
    base->destroy = data_display_widget_destroy;
    base->layout = data_display_widget_layout;
    
    // Initialize arrays
    base->child_capacity = 8; // 4 labels + 4 values
    base->children = calloc(base->child_capacity, sizeof(Widget*));
    if (!base->children) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate children array for data display widget");
        free(data_widget);
        return NULL;
    }
    base->event_capacity = 0;
    base->subscribed_events = NULL;
    
    // Set properties
    data_widget->label_font = label_font;
    data_widget->value_font = value_font;
    data_widget->line_spacing = 30;
    data_widget->label_width = 80;
    
    // Create text widgets for labels and values
    char widget_id[128];
    SDL_Color label_color = {180, 180, 180, 255};
    SDL_Color value_color = {255, 255, 255, 255};
    
    // Name
    snprintf(widget_id, sizeof(widget_id), "%s_name_label", id);
    TextWidget* name_label = (TextWidget*)text_widget_create(widget_id, "Name:", label_font);
    text_widget_set_color((Widget*)name_label, label_color);
    text_widget_set_alignment((Widget*)name_label, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)name_label);
    
    snprintf(widget_id, sizeof(widget_id), "%s_name_value", id);
    data_widget->name_widget = (TextWidget*)text_widget_create(widget_id, "", value_font);
    text_widget_set_color((Widget*)data_widget->name_widget, value_color);
    text_widget_set_alignment((Widget*)data_widget->name_widget, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)data_widget->name_widget);
    
    // Email
    snprintf(widget_id, sizeof(widget_id), "%s_email_label", id);
    TextWidget* email_label = (TextWidget*)text_widget_create(widget_id, "Email:", label_font);
    text_widget_set_color((Widget*)email_label, label_color);
    text_widget_set_alignment((Widget*)email_label, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)email_label);
    
    snprintf(widget_id, sizeof(widget_id), "%s_email_value", id);
    data_widget->email_widget = (TextWidget*)text_widget_create(widget_id, "", value_font);
    text_widget_set_color((Widget*)data_widget->email_widget, value_color);
    text_widget_set_alignment((Widget*)data_widget->email_widget, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)data_widget->email_widget);
    
    // Phone
    snprintf(widget_id, sizeof(widget_id), "%s_phone_label", id);
    TextWidget* phone_label = (TextWidget*)text_widget_create(widget_id, "Phone:", label_font);
    text_widget_set_color((Widget*)phone_label, label_color);
    text_widget_set_alignment((Widget*)phone_label, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)phone_label);
    
    snprintf(widget_id, sizeof(widget_id), "%s_phone_value", id);
    data_widget->phone_widget = (TextWidget*)text_widget_create(widget_id, "", value_font);
    text_widget_set_color((Widget*)data_widget->phone_widget, value_color);
    text_widget_set_alignment((Widget*)data_widget->phone_widget, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)data_widget->phone_widget);
    
    // Location
    snprintf(widget_id, sizeof(widget_id), "%s_location_label", id);
    TextWidget* location_label = (TextWidget*)text_widget_create(widget_id, "Location:", label_font);
    text_widget_set_color((Widget*)location_label, label_color);
    text_widget_set_alignment((Widget*)location_label, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)location_label);
    
    snprintf(widget_id, sizeof(widget_id), "%s_location_value", id);
    data_widget->location_widget = (TextWidget*)text_widget_create(widget_id, "", value_font);
    text_widget_set_color((Widget*)data_widget->location_widget, value_color);
    text_widget_set_alignment((Widget*)data_widget->location_widget, TEXT_ALIGN_LEFT);
    widget_add_child(base, (Widget*)data_widget->location_widget);
    
    // Set initial size
    base->bounds.w = 400;
    base->bounds.h = 4 * data_widget->line_spacing;
    
    return (Widget*)data_widget;
}

void data_display_widget_set_user_data(Widget* widget, const char* name, 
                                      const char* email, const char* phone, 
                                      const char* city, const char* country) {
    if (!widget) return;
    DataDisplayWidget* data_widget = (DataDisplayWidget*)widget;
    
    // Update text widgets
    text_widget_set_text((Widget*)data_widget->name_widget, name ? name : "");
    text_widget_set_text((Widget*)data_widget->email_widget, email ? email : "");
    text_widget_set_text((Widget*)data_widget->phone_widget, phone ? phone : "");
    
    // Combine city and country for location
    char location[256] = "";
    if (city && country) {
        snprintf(location, sizeof(location), "%s, %s", city, country);
    } else if (city) {
        strncpy(location, city, sizeof(location) - 1);
    } else if (country) {
        strncpy(location, country, sizeof(location) - 1);
    }
    text_widget_set_text((Widget*)data_widget->location_widget, location);
    
    widget->state_flags |= WIDGET_STATE_DIRTY;
}

void data_display_widget_clear(Widget* widget) {
    data_display_widget_set_user_data(widget, "", "", "", "", "");
}

static void data_display_widget_layout(Widget* widget) {
    if (!widget) return;
    DataDisplayWidget* data_widget = (DataDisplayWidget*)widget;
    
    int y = widget->bounds.y;
    int x_label = widget->bounds.x;
    int x_value = widget->bounds.x + data_widget->label_width;
    
    // Layout each label/value pair
    for (size_t i = 0; i < widget->child_count; i += 2) {
        if (i < widget->child_count && widget->children[i]) {
            // Label
            widget->children[i]->bounds.x = x_label;
            widget->children[i]->bounds.y = y;
            widget->children[i]->bounds.w = data_widget->label_width;
            widget->children[i]->bounds.h = data_widget->line_spacing;
        }
        
        if (i + 1 < widget->child_count && widget->children[i + 1]) {
            // Value
            widget->children[i + 1]->bounds.x = x_value;
            widget->children[i + 1]->bounds.y = y;
            widget->children[i + 1]->bounds.w = widget->bounds.w - data_widget->label_width;
            widget->children[i + 1]->bounds.h = data_widget->line_spacing;
        }
        
        y += data_widget->line_spacing;
    }
}

static PkError data_display_widget_render(Widget* widget, SDL_Renderer* renderer) {
    PK_CHECK_ERROR_WITH_CONTEXT(widget != NULL, PK_ERROR_NULL_PARAM,
                               "widget is NULL in data_display_widget_render");
    PK_CHECK_ERROR_WITH_CONTEXT(renderer != NULL, PK_ERROR_NULL_PARAM,
                               "renderer is NULL in data_display_widget_render");
    
    // Layout children
    data_display_widget_layout(widget);
    
    // Render all child text widgets
    for (size_t i = 0; i < widget->child_count; i++) {
        if (widget->children[i] && widget->children[i]->render) {
            PkError err = widget->children[i]->render(widget->children[i], renderer);
            if (err != PK_OK) {
                return err;
            }
        }
    }
    
    return PK_OK;
}

static void data_display_widget_destroy(Widget* widget) {
    // Children are destroyed by widget_destroy
}