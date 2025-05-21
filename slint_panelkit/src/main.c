/*********************
 * INCLUDES
 *********************/
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>

/*********************
 * DEFINES
 *********************/
#define DISP_BUF_SIZE (640 * 480)

/*********************
 * STATIC VARIABLES
 *********************/
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint32_t *buffer;
static lv_display_t *display;
static lv_draw_buf_t disp_buf;
static lv_color_t buf1[DISP_BUF_SIZE];
static lv_indev_t *mouse_indev;
static volatile bool running = true;

/*********************
 * STATIC PROTOTYPES
 *********************/
static void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data);
static int tick_thread(void *data);
static int event_thread(void *data);
static void create_demo_ui(void);

// Event handlers
static void blue_button_event_handler(lv_event_t *e);
static void random_button_event_handler(lv_event_t *e);
static void date_button_event_handler(lv_event_t *e);
static void time_timer_cb(lv_timer_t *t);

/**********************
 * SDL DISPLAY DRIVER
 **********************/
static void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // Calculate dimensions
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    
    // Get color format (SDL uses RGBA, so we use LV_COLOR_FORMAT_ARGB8888)
    uint8_t *buf_act = px_map;
    
    // Copy pixels to the SDL buffer
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint32_t src_idx = y * w + x;
            buffer[(area->y1 + y) * 640 + area->x1 + x] = *(uint32_t *)(buf_act + src_idx * 4);
        }
    }
    
    // Update the texture and render 
    SDL_UpdateTexture(texture, NULL, buffer, 640 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    
    // Indicate to LVGL that we're done
    lv_display_flush_ready(disp);
}

/**********************
 * SDL INPUT DRIVER
 **********************/
static void mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    // Get mouse position and button state from SDL
    int x, y;
    uint32_t buttons = SDL_GetMouseState(&x, &y);
    
    // Set the input data
    data->point.x = x;
    data->point.y = y;
    
    // The left mouse button is pressed
    data->state = (buttons & SDL_BUTTON(1)) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/**********************
 * TIMER THREAD
 **********************/
static int tick_thread(void *data)
{
    while (running) {
        // Tell LVGL that 5ms have elapsed
        lv_tick_inc(5);
        SDL_Delay(5);
    }
    return 0;
}

/**********************
 * EVENT THREAD
 **********************/
static int event_thread(void *data)
{
    SDL_Event event;
    
    while (running) {
        // Check for SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_Delay(10);
    }
    return 0;
}

/**********************
 * EVENT HANDLERS
 **********************/

// Blue button event handler
static void blue_button_event_handler(lv_event_t *e)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x2980b9), LV_PART_MAIN);
}

// Random button event handler
static void random_button_event_handler(lv_event_t *e)
{
    uint8_t r = rand() % 256;
    uint8_t g = rand() % 256;
    uint8_t b = rand() % 256;
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_make(r, g, b), LV_PART_MAIN);
}

// Time update timer callback
static void time_timer_cb(lv_timer_t *t)
{
    lv_obj_t *label = (lv_obj_t *)t->user_data;
    
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    
    char time_str[9];
    char date_str[12];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);
    strftime(date_str, sizeof(date_str), "%Y-%b-%d", time_info);
    
    char datetime_str[32];
    snprintf(datetime_str, sizeof(datetime_str), "%s\n%s", time_str, date_str);
    lv_label_set_text(label, datetime_str);
    lv_obj_center(label);
}

// Date button event handler
static void date_button_event_handler(lv_event_t *e)
{
    static bool inverted = false;
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    
    // Toggle inverted state
    inverted = !inverted;
    
    if (inverted) {
        // Invert button colors
        lv_obj_set_style_bg_color(btn, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0x8e44ad), LV_PART_MAIN);
    } else {
        // Restore original colors
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x8e44ad), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    }
}

/**********************
 * UI CREATION
 **********************/
static void create_demo_ui(void)
{
    // Get the active screen
    lv_obj_t *scr = lv_scr_act();
    
    // Set screen background color (light gray)
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf0f0f0), LV_PART_MAIN);
    
    // Create a container for scrolling
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 640, 480);
    lv_obj_center(cont);
    
    // Enable scrolling
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    
    // Use flex layout for container
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_row(cont, 20, LV_PART_MAIN);
    
    // Create a blue button
    lv_obj_t *btn1 = lv_button_create(cont);
    lv_obj_set_size(btn1, 640/2, 480*2/3);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0x2980b9), LV_PART_MAIN);
    lv_obj_set_style_radius(btn1, 10, LV_PART_MAIN);
    
    // Add label to the button
    lv_obj_t *label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Blue");
    lv_obj_center(label1);
    
    // Event handler for the blue button (changes screen background)
    lv_obj_add_event_cb(btn1, blue_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Create a green button
    lv_obj_t *btn2 = lv_button_create(cont);
    lv_obj_set_size(btn2, 640/2, 480*2/3);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0x16a085), LV_PART_MAIN);
    lv_obj_set_style_radius(btn2, 10, LV_PART_MAIN);
    
    // Add label to the button
    lv_obj_t *label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Random");
    lv_obj_center(label2);
    
    // Event handler for the random button (changes screen to random color)
    lv_obj_add_event_cb(btn2, random_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Create a date/time button
    lv_obj_t *btn3 = lv_button_create(cont);
    lv_obj_set_size(btn3, 640/2, 480*2/3);
    lv_obj_set_style_bg_color(btn3, lv_color_hex(0x8e44ad), LV_PART_MAIN);
    lv_obj_set_style_radius(btn3, 10, LV_PART_MAIN);
    
    // Add label to the button
    lv_obj_t *label3 = lv_label_create(btn3);
    
    // Use the current time for the label
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    char time_str[9]; // HH:MM:SS\0
    char date_str[12]; // YYYY-MMM-DD\0
    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);
    strftime(date_str, sizeof(date_str), "%Y-%b-%d", time_info);
    
    // Set the label text
    char datetime_str[32];
    snprintf(datetime_str, sizeof(datetime_str), "%s\n%s", time_str, date_str);
    lv_label_set_text(label3, datetime_str);
    lv_obj_center(label3);
    
    // Create a timer to update the date/time every second
    lv_timer_t *timer = lv_timer_create(time_timer_cb, 1000, label3);
    
    // Event handler for the date button (inverts colors)
    lv_obj_add_event_cb(btn3, date_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Add additional buttons for scrolling
    lv_color_t colors[] = {
        lv_color_hex(0xe74c3c),
        lv_color_hex(0xf39c12),
        lv_color_hex(0x27ae60),
        lv_color_hex(0x2980b9),
        lv_color_hex(0x8e44ad),
        lv_color_hex(0x2c3e50)
    };
    
    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 640/2, 480*2/3);
        lv_obj_set_style_bg_color(btn, colors[i], LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        
        lv_obj_t *label = lv_label_create(btn);
        char buf[16];
        snprintf(buf, sizeof(buf), "Button #%d", i + 4);
        lv_label_set_text(label, buf);
        lv_obj_center(label);
    }
}

/**********************
 * MAIN FUNCTION
 **********************/
int main(int argc, char **argv)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL initialization error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create window and renderer
    window = SDL_CreateWindow("PanelKit",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             640, 480, 0);
    if (!window) {
        printf("Window creation error: %s\n", SDL_GetError());
        return 2;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_STATIC, 640, 480);
    buffer = (uint32_t *)calloc(640 * 480, sizeof(uint32_t));
    
    if (!renderer || !texture || !buffer) {
        printf("Renderer creation error\n");
        return 3;
    }
    
    // Initialize LVGL
    lv_init();
    
    // Initialize the display buffer
    lv_draw_buf_init(&disp_buf, 640, 480, LV_COLOR_FORMAT_ARGB8888);
    
    // Register a display driver
    display = lv_display_create(640, 480);
    lv_display_set_flush_cb(display, display_flush_cb);
    lv_display_set_buffers(display, &disp_buf, NULL, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // Register a mouse driver
    mouse_indev = lv_indev_create();
    lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(mouse_indev, mouse_read_cb);
    
    // Initialize random number generator for random color button
    srand(time(NULL));
    
    // Create threads for tick and event handling
    SDL_CreateThread(tick_thread, "tick", NULL);
    SDL_CreateThread(event_thread, "event", NULL);
    
    // Create the demo UI
    create_demo_ui();
    
    // Main loop
    while (running) {
        // Let LVGL handle its tasks
        lv_timer_handler();
        
        // Sleep to reduce CPU usage
        usleep(5000); // 5ms pause
    }
    
    // Cleanup
    free(buffer);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}