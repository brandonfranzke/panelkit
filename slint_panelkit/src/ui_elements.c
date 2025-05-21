#include "ui_elements.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// UI elements
static lv_obj_t *scroll_container;  // Scrollable container
static lv_obj_t *blue_btn;          // Blue button
static lv_obj_t *random_btn;        // Random color button
static lv_obj_t *date_btn;          // Date/time button
static lv_obj_t *date_label;        // Text label for date/time button

// Application state
static lv_color_t bg_color;          // Background color
static bool date_inverted = false;   // Date button inverted state
static lv_timer_t *time_update_timer; // Timer for updating time display

/**
 * Create a button with specified dimensions, colors, and label text
 */
static lv_obj_t *create_button(lv_obj_t *parent, const char *text, lv_color_t bg_color)
{
    // Create button object
    lv_obj_t *btn = lv_btn_create(parent);
    
    // Set size (50% width, 2/3 height of screen)
    lv_coord_t btn_width = 640 / 2;  // Half of screen width
    lv_coord_t btn_height = 480 * 2 / 3;  // 2/3 of screen height
    
    lv_obj_set_size(btn, btn_width, btn_height);
    
    // Set button style
    lv_obj_set_style_bg_color(btn, bg_color, 0);
    lv_obj_set_style_radius(btn, 10, 0);  // Rounded corners
    
    // Center button in parent
    lv_obj_set_style_align(btn, LV_ALIGN_CENTER, 0);
    
    // Create label on the button
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);  // Center the label on the button
    
    return btn;
}

// Blue button event callback
void blue_button_event_cb(lv_event_t *e)
{
    lv_obj_t *screen = lv_scr_act();
    
    // Set blue background
    bg_color = lv_color_hex(0x2980b9);  // Blue color (#2980b9)
    lv_obj_set_style_bg_color(screen, bg_color, 0);
    
    printf("Blue button clicked\n");
}

// Random button event callback
void random_button_event_cb(lv_event_t *e)
{
    lv_obj_t *screen = lv_scr_act();
    
    // Generate random RGB color
    uint8_t r = rand() % 256;
    uint8_t g = rand() % 256;
    uint8_t b = rand() % 256;
    
    // Set random background color
    bg_color = lv_color_make(r, g, b);
    lv_obj_set_style_bg_color(screen, bg_color, 0);
    
    printf("Random button clicked: RGB(%d, %d, %d)\n", r, g, b);
}

// Date button event callback
void date_button_event_cb(lv_event_t *e)
{
    // Toggle date button inverted state
    date_inverted = !date_inverted;
    
    if (date_inverted) {
        // Invert button colors
        lv_obj_set_style_bg_color(date_btn, lv_color_white(), 0);
        lv_obj_set_style_text_color(date_label, lv_color_hex(0x8e44ad), 0);
    } else {
        // Restore button colors
        lv_obj_set_style_bg_color(date_btn, lv_color_hex(0x8e44ad), 0);
        lv_obj_set_style_text_color(date_label, lv_color_white(), 0);
    }
    
    printf("Date button clicked, inverted: %s\n", date_inverted ? "true" : "false");
}

// Timer callback to update time display
static void time_update_timer_cb(lv_timer_t *timer)
{
    update_time_display();
}

// Update the time and date display
void update_time_display(void)
{
    // Get current time
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    
    // Format time and date
    char time_str[9];  // HH:MM:SS\0
    char date_str[12]; // YYYY-MMM-DD\0
    
    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);
    strftime(date_str, sizeof(date_str), "%Y-%b-%d", time_info);
    
    // Combine time and date with newline
    char datetime_str[32];
    snprintf(datetime_str, sizeof(datetime_str), "%s\n%s", time_str, date_str);
    
    // Update label
    lv_label_set_text(date_label, datetime_str);
    
    // Center the label on the button
    lv_obj_center(date_label);
}

// Initialize the UI elements
void ui_init(lv_obj_t *screen)
{
    // Set default background color
    bg_color = lv_color_hex(0xf0f0f0);  // Light gray background
    lv_obj_set_style_bg_color(screen, bg_color, 0);
    
    // Create a scrollable container
    scroll_container = lv_obj_create(screen);
    lv_obj_set_size(scroll_container, 640, 480);  // Full screen size
    lv_obj_center(scroll_container);
    
    // Set layout for the scrollable container
    lv_obj_set_flex_flow(scroll_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Set scroll properties
    lv_obj_set_style_pad_all(scroll_container, 20, 0);
    lv_obj_set_style_pad_row(scroll_container, 20, 0);  // Spacing between rows
    lv_obj_set_scrollbar_mode(scroll_container, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(scroll_container, LV_DIR_VER);  // Vertical scrolling only
    
    // Create blue button
    blue_btn = create_button(scroll_container, "Blue", lv_color_hex(0x2980b9));
    lv_obj_add_event_cb(blue_btn, blue_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create random button
    random_btn = create_button(scroll_container, "Random", lv_color_hex(0x16a085));
    lv_obj_add_event_cb(random_btn, random_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create date button with default text (will be updated by timer)
    date_btn = create_button(scroll_container, "", lv_color_hex(0x8e44ad));
    date_label = lv_obj_get_child(date_btn, 0);  // Get the label created by create_button
    lv_obj_add_event_cb(date_btn, date_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create additional buttons (as in the original design)
    lv_color_t colors[] = {
        lv_color_hex(0xe74c3c),
        lv_color_hex(0xf39c12),
        lv_color_hex(0x27ae60),
        lv_color_hex(0x2980b9),
        lv_color_hex(0x8e44ad),
        lv_color_hex(0x2c3e50)
    };
    
    char btn_text[16];
    for (int i = 0; i < 6; i++) {
        snprintf(btn_text, sizeof(btn_text), "Button #%d", i + 4);
        lv_obj_t *btn = create_button(scroll_container, btn_text, colors[i]);
    }
    
    // Initial time update
    update_time_display();
    
    // Create timer for updating time display (every second)
    time_update_timer = lv_timer_create(time_update_timer_cb, 1000, NULL);
}