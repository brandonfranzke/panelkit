#ifndef EVENT_SYSTEM_TYPED_H
#define EVENT_SYSTEM_TYPED_H

#include "event_system.h"
#include "event_types.h"
#include <stdbool.h>

// Forward declaration
typedef struct EventSystem EventSystem;

// Handler typedefs for each event type
typedef void (*button_pressed_handler)(const ButtonEventData* data, void* context);
typedef void (*page_changed_handler)(const PageChangeEventData* data, void* context);
typedef void (*page_transition_handler)(int target_page, void* context);
typedef void (*api_refresh_requested_handler)(uint32_t timestamp, void* context);
typedef void (*api_state_changed_handler)(const ApiStateChangeData* data, void* context);
typedef void (*api_user_data_updated_handler)(const void* user_data, size_t size, void* context);
typedef void (*api_refresh_handler)(const ApiRefreshData* data, void* context);
typedef void (*weather_request_handler)(const char* location, void* context);
typedef void (*touch_down_handler)(const TouchEventData* data, void* context);
typedef void (*touch_up_handler)(const TouchEventData* data, void* context);

// Typed publish functions
bool event_publish_button_pressed(EventSystem* system, const ButtonEventData* data);
bool event_publish_page_changed(EventSystem* system, const PageChangeEventData* data);
bool event_publish_page_transition(EventSystem* system, int target_page);
bool event_publish_api_refresh_requested(EventSystem* system, uint32_t timestamp);
bool event_publish_api_state_changed(EventSystem* system, const ApiStateChangeData* data);
bool event_publish_api_user_data_updated(EventSystem* system, const void* user_data, size_t size);
bool event_publish_api_refresh(EventSystem* system, const ApiRefreshData* data);
bool event_publish_weather_request(EventSystem* system, const char* location);
bool event_publish_touch_down(EventSystem* system, const TouchEventData* data);
bool event_publish_touch_up(EventSystem* system, const TouchEventData* data);

// Typed subscribe functions
bool event_subscribe_button_pressed(EventSystem* system, button_pressed_handler handler, void* context);
bool event_subscribe_page_changed(EventSystem* system, page_changed_handler handler, void* context);
bool event_subscribe_page_transition(EventSystem* system, page_transition_handler handler, void* context);
bool event_subscribe_api_refresh_requested(EventSystem* system, api_refresh_requested_handler handler, void* context);
bool event_subscribe_api_state_changed(EventSystem* system, api_state_changed_handler handler, void* context);
bool event_subscribe_api_user_data_updated(EventSystem* system, api_user_data_updated_handler handler, void* context);
bool event_subscribe_api_refresh(EventSystem* system, api_refresh_handler handler, void* context);
bool event_subscribe_weather_request(EventSystem* system, weather_request_handler handler, void* context);
bool event_subscribe_touch_down(EventSystem* system, touch_down_handler handler, void* context);
bool event_subscribe_touch_up(EventSystem* system, touch_up_handler handler, void* context);

// Typed unsubscribe functions
bool event_unsubscribe_button_pressed(EventSystem* system, button_pressed_handler handler);
bool event_unsubscribe_page_changed(EventSystem* system, page_changed_handler handler);
bool event_unsubscribe_page_transition(EventSystem* system, page_transition_handler handler);
bool event_unsubscribe_api_refresh_requested(EventSystem* system, api_refresh_requested_handler handler);
bool event_unsubscribe_api_state_changed(EventSystem* system, api_state_changed_handler handler);
bool event_unsubscribe_api_user_data_updated(EventSystem* system, api_user_data_updated_handler handler);
bool event_unsubscribe_api_refresh(EventSystem* system, api_refresh_handler handler);
bool event_unsubscribe_weather_request(EventSystem* system, weather_request_handler handler);
bool event_unsubscribe_touch_down(EventSystem* system, touch_down_handler handler);
bool event_unsubscribe_touch_up(EventSystem* system, touch_up_handler handler);

#endif // EVENT_SYSTEM_TYPED_H