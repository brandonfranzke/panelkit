/**
 * @file input_debug.h
 * @brief Input system debugging and introspection
 */

#ifndef PANELKIT_INPUT_DEBUG_H
#define PANELKIT_INPUT_DEBUG_H

#include "input_handler.h"

/* Log detailed input system state */
void input_debug_log_state(InputHandler* handler);

/* Log SDL input subsystem state */
void input_debug_log_sdl_state(void);

/* Log evdev device capabilities */
void input_debug_log_device_caps(const char* device_path);

/* Monitor and log input events in real-time */
void input_debug_start_monitor(InputHandler* handler);
void input_debug_stop_monitor(InputHandler* handler);

#endif /* PANELKIT_INPUT_DEBUG_H */