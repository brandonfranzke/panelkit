#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "lvgl/lvgl.h"

/**
 * Initialize the UI elements
 * @param screen Pointer to the main screen object
 */
void ui_init(lv_obj_t *screen);

/**
 * Event handler for the blue button
 * @param e Pointer to the event
 */
void blue_button_event_cb(lv_event_t *e);

/**
 * Event handler for the random button
 * @param e Pointer to the event
 */
void random_button_event_cb(lv_event_t *e);

/**
 * Event handler for the date button
 * @param e Pointer to the event
 */
void date_button_event_cb(lv_event_t *e);

/**
 * Update the time and date display
 */
void update_time_display(void);

#endif /* UI_ELEMENTS_H */