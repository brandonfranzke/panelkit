#ifndef WIDGET_INTEGRATION_INTERNAL_H
#define WIDGET_INTEGRATION_INTERNAL_H

#include "widget_integration.h"

// Internal functions shared between widget_integration modules

// State management functions (from widget_integration_state.c)
void widget_integration_init_app_state(WidgetIntegration* integration);

// Event handling callbacks (from widget_integration_events.c)
void widget_integration_page_changed_callback(int from_page, int to_page, void* user_data);

// Widget creation helpers (from widget_integration_widgets.c)
void widget_integration_populate_page_widgets(WidgetIntegration* integration);

#endif // WIDGET_INTEGRATION_INTERNAL_H