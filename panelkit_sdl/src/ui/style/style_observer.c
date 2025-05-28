#include "style_observer.h"
#include "style_templates.h"
#include "core/logger.h"
#include "core/error.h"
#include <stdlib.h>
#include <string.h>

// Observer entry for widgets
typedef struct WidgetObserver {
    ObserverHandle handle;
    Widget* widget;
    StyleChangeCallback callback;
    void* user_data;
    struct WidgetObserver* next;
} WidgetObserver;

// Observer entry for templates
typedef struct TemplateObserver {
    ObserverHandle handle;
    char template_name[64];
    TemplateChangeCallback callback;
    void* user_data;
    struct TemplateObserver* next;
} TemplateObserver;

// Pending notification for batch mode
typedef struct PendingNotification {
    enum { NOTIFY_WIDGET, NOTIFY_TEMPLATE } type;
    union {
        struct {
            Widget* widget;
            Style* old_style;
            Style* new_style;
        } widget;
        struct {
            char template_name[64];
            Style* new_template;
        } template;
    } data;
    struct PendingNotification* next;
} PendingNotification;

// Global observer state
static struct {
    WidgetObserver* widget_observers;
    TemplateObserver* template_observers;
    PendingNotification* pending_notifications;
    uint32_t next_handle;
    bool is_batching;
} g_observer_state = {0};

// ============================================================================
// Widget Observers
// ============================================================================

ObserverHandle style_observer_register_widget(Widget* widget, 
                                             StyleChangeCallback callback, 
                                             void* user_data) {
    if (!widget || !callback) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL widget or callback in style_observer_register_widget");
        return 0;
    }
    
    WidgetObserver* observer = calloc(1, sizeof(WidgetObserver));
    if (!observer) {
        pk_set_last_error(PK_ERROR_OUT_OF_MEMORY);
        return 0;
    }
    
    observer->handle = ++g_observer_state.next_handle;
    observer->widget = widget;
    observer->callback = callback;
    observer->user_data = user_data;
    observer->next = g_observer_state.widget_observers;
    g_observer_state.widget_observers = observer;
    
    log_debug("Registered style observer %u for widget '%s'", 
              observer->handle, widget->id);
    
    return observer->handle;
}

void style_observer_unregister_widget(Widget* widget, ObserverHandle handle) {
    if (!widget || !handle) return;
    
    WidgetObserver** current = &g_observer_state.widget_observers;
    while (*current) {
        if ((*current)->widget == widget && (*current)->handle == handle) {
            WidgetObserver* to_remove = *current;
            *current = (*current)->next;
            
            log_debug("Unregistered style observer %u for widget '%s'", 
                      handle, widget->id);
            
            free(to_remove);
            return;
        }
        current = &(*current)->next;
    }
}

void style_observer_notify_widget(Widget* widget, const Style* old_style, const Style* new_style) {
    if (!widget) return;
    
    // If batching, queue notification
    if (g_observer_state.is_batching) {
        PendingNotification* pending = calloc(1, sizeof(PendingNotification));
        if (!pending) return;
        
        pending->type = NOTIFY_WIDGET;
        pending->data.widget.widget = widget;
        pending->data.widget.old_style = old_style ? style_create_from(old_style) : NULL;
        pending->data.widget.new_style = new_style ? style_create_from(new_style) : NULL;
        
        pending->next = g_observer_state.pending_notifications;
        g_observer_state.pending_notifications = pending;
        return;
    }
    
    // Notify all observers for this widget
    WidgetObserver* observer = g_observer_state.widget_observers;
    while (observer) {
        if (observer->widget == widget) {
            observer->callback(widget, old_style, new_style, observer->user_data);
        }
        observer = observer->next;
    }
}

// ============================================================================
// Template Observers
// ============================================================================

ObserverHandle style_observer_register_template(const char* template_name,
                                               TemplateChangeCallback callback,
                                               void* user_data) {
    if (!template_name || !callback) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL template_name or callback in style_observer_register_template");
        return 0;
    }
    
    TemplateObserver* observer = calloc(1, sizeof(TemplateObserver));
    if (!observer) {
        pk_set_last_error(PK_ERROR_OUT_OF_MEMORY);
        return 0;
    }
    
    observer->handle = ++g_observer_state.next_handle;
    strncpy(observer->template_name, template_name, sizeof(observer->template_name) - 1);
    observer->callback = callback;
    observer->user_data = user_data;
    observer->next = g_observer_state.template_observers;
    g_observer_state.template_observers = observer;
    
    log_debug("Registered template observer %u for '%s'", 
              observer->handle, template_name);
    
    return observer->handle;
}

void style_observer_unregister_template(ObserverHandle handle) {
    if (!handle) return;
    
    TemplateObserver** current = &g_observer_state.template_observers;
    while (*current) {
        if ((*current)->handle == handle) {
            TemplateObserver* to_remove = *current;
            *current = (*current)->next;
            
            log_debug("Unregistered template observer %u", handle);
            
            free(to_remove);
            return;
        }
        current = &(*current)->next;
    }
}

void style_observer_notify_template(const char* template_name, const Style* new_template) {
    if (!template_name) return;
    
    // If batching, queue notification
    if (g_observer_state.is_batching) {
        PendingNotification* pending = calloc(1, sizeof(PendingNotification));
        if (!pending) return;
        
        pending->type = NOTIFY_TEMPLATE;
        strncpy(pending->data.template.template_name, template_name, 
                sizeof(pending->data.template.template_name) - 1);
        pending->data.template.new_template = new_template ? style_create_from(new_template) : NULL;
        
        pending->next = g_observer_state.pending_notifications;
        g_observer_state.pending_notifications = pending;
        return;
    }
    
    // Notify all observers for this template
    TemplateObserver* observer = g_observer_state.template_observers;
    while (observer) {
        if (strcmp(observer->template_name, template_name) == 0) {
            observer->callback(template_name, new_template, observer->user_data);
        }
        observer = observer->next;
    }
}

// ============================================================================
// Batch Updates
// ============================================================================

void style_observer_begin_batch(void) {
    g_observer_state.is_batching = true;
    log_debug("Beginning style batch update");
}

void style_observer_end_batch(void) {
    if (!g_observer_state.is_batching) return;
    
    g_observer_state.is_batching = false;
    log_debug("Ending style batch update");
    
    // Process all pending notifications
    PendingNotification* pending = g_observer_state.pending_notifications;
    g_observer_state.pending_notifications = NULL;
    
    while (pending) {
        PendingNotification* current = pending;
        pending = pending->next;
        
        switch (current->type) {
            case NOTIFY_WIDGET:
                style_observer_notify_widget(current->data.widget.widget,
                                           current->data.widget.old_style,
                                           current->data.widget.new_style);
                if (current->data.widget.old_style) {
                    style_destroy(current->data.widget.old_style);
                }
                if (current->data.widget.new_style) {
                    style_destroy(current->data.widget.new_style);
                }
                break;
                
            case NOTIFY_TEMPLATE:
                style_observer_notify_template(current->data.template.template_name,
                                             current->data.template.new_template);
                if (current->data.template.new_template) {
                    style_destroy(current->data.template.new_template);
                }
                break;
        }
        
        free(current);
    }
}

bool style_observer_is_batching(void) {
    return g_observer_state.is_batching;
}

// ============================================================================
// Utility Functions
// ============================================================================

// Helper to find widgets using a template
// TODO: Implement when widgets store their template name
// static void find_template_users(Widget* root, const char* template_name, 
//                                Widget*** users, size_t* count, size_t* capacity) {
//     if (!root) return;
//     
//     // Check if this widget uses the template
//     // Note: This would require widgets to store their template name
//     // For now, this is a placeholder implementation
//     
//     // Recurse to children
//     for (size_t i = 0; i < root->child_count; i++) {
//         find_template_users(root->children[i], template_name, users, count, capacity);
//     }
// }

void style_observer_update_template_users(const char* template_name, const Style* new_template) {
    if (!template_name || !new_template) return;
    
    log_debug("Updating all widgets using template '%s'", template_name);
    
    // Update the template in the registry
    style_template_register(template_name, new_template);
    
    // Notify template observers
    style_observer_notify_template(template_name, new_template);
    
    // In a full implementation, we would:
    // 1. Find all widgets using this template
    // 2. Update their styles
    // 3. Notify widget observers
}

void style_observer_clear_all(void) {
    // Clear widget observers
    WidgetObserver* widget_obs = g_observer_state.widget_observers;
    while (widget_obs) {
        WidgetObserver* next = widget_obs->next;
        free(widget_obs);
        widget_obs = next;
    }
    g_observer_state.widget_observers = NULL;
    
    // Clear template observers
    TemplateObserver* template_obs = g_observer_state.template_observers;
    while (template_obs) {
        TemplateObserver* next = template_obs->next;
        free(template_obs);
        template_obs = next;
    }
    g_observer_state.template_observers = NULL;
    
    // Clear pending notifications
    PendingNotification* pending = g_observer_state.pending_notifications;
    while (pending) {
        PendingNotification* next = pending->next;
        if (pending->type == NOTIFY_WIDGET) {
            if (pending->data.widget.old_style) {
                style_destroy(pending->data.widget.old_style);
            }
            if (pending->data.widget.new_style) {
                style_destroy(pending->data.widget.new_style);
            }
        } else if (pending->type == NOTIFY_TEMPLATE) {
            if (pending->data.template.new_template) {
                style_destroy(pending->data.template.new_template);
            }
        }
        free(pending);
        pending = next;
    }
    g_observer_state.pending_notifications = NULL;
    
    g_observer_state.is_batching = false;
    
    log_debug("Cleared all style observers");
}