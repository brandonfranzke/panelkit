#ifndef PK_STYLE_OBSERVER_H
#define PK_STYLE_OBSERVER_H

#include "../widget.h"
#include "style_core.h"

// Observer callback types
typedef void (*StyleChangeCallback)(Widget* widget, const Style* old_style, const Style* new_style, void* user_data);
typedef void (*TemplateChangeCallback)(const char* template_name, const Style* new_template, void* user_data);

// Observer handle for unsubscribing
typedef uint32_t ObserverHandle;

// ============================================================================
// Widget Style Observers
// ============================================================================

// Register observer for style changes on a specific widget
ObserverHandle style_observer_register_widget(Widget* widget, 
                                             StyleChangeCallback callback, 
                                             void* user_data);

// Unregister widget observer
void style_observer_unregister_widget(Widget* widget, ObserverHandle handle);

// Notify observers of style change (called automatically by widget_set_style)
void style_observer_notify_widget(Widget* widget, const Style* old_style, const Style* new_style);

// ============================================================================
// Template Observers
// ============================================================================

// Register observer for template registry changes
ObserverHandle style_observer_register_template(const char* template_name,
                                               TemplateChangeCallback callback,
                                               void* user_data);

// Unregister template observer
void style_observer_unregister_template(ObserverHandle handle);

// Notify observers of template change
void style_observer_notify_template(const char* template_name, const Style* new_template);

// ============================================================================
// Batch Updates
// ============================================================================

// Begin batch update - suppresses notifications until end
void style_observer_begin_batch(void);

// End batch update - sends all pending notifications
void style_observer_end_batch(void);

// Check if currently in batch mode
bool style_observer_is_batching(void);

// ============================================================================
// Utility Functions
// ============================================================================

// Update all widgets using a specific template
void style_observer_update_template_users(const char* template_name, const Style* new_template);

// Clear all observers (cleanup)
void style_observer_clear_all(void);

#endif // PK_STYLE_OBSERVER_H