#include "platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Display buffer size
#define DISP_BUF_SIZE (LV_HOR_RES_MAX * 10)

#if LV_USE_SDL
    #include <SDL2/SDL.h>
    static SDL_Window *window;
    static SDL_Renderer *renderer;
    static SDL_Texture *texture;
    static uint32_t *buffer;
#endif

// Display buffer(s)
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf1[DISP_BUF_SIZE];

// Display device driver
static lv_disp_drv_t disp_drv;

// Input device driver
static lv_indev_drv_t indev_drv;

#if LV_USE_SDL
// SDL display driver flush callback
static void sdl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    // Convert to SDL texture format and copy to buffer
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    // Map colors
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint32_t color_value =
                ((color_p->ch.red   << 16) & 0x00ff0000) |
                ((color_p->ch.green << 8)  & 0x0000ff00) |
                ((color_p->ch.blue)        & 0x000000ff);
                
            buffer[(area->y1 + y) * LV_HOR_RES_MAX + area->x1 + x] = color_value;
            color_p++;
        }
    }
    
    // Update SDL texture from buffer and render
    SDL_UpdateTexture(texture, NULL, buffer, LV_HOR_RES_MAX * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    
    // Important: Notify LVGL that we're done with the buffer
    lv_disp_flush_ready(disp_drv);
}

// SDL input driver read callback
static void sdl_mouse_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    // Get mouse position and button state from SDL
    int x, y;
    uint32_t buttons = SDL_GetMouseState(&x, &y);
    
    // Set the input data
    data->state = (buttons & SDL_BUTTON(1)) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->point.x = x;
    data->point.y = y;
}

#endif // LV_USE_SDL

int platform_init(void)
{
    // Initialize LVGL
    lv_init();
    
#if LV_USE_SDL
    // Initialize SDL for display and input
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return -1;
    }
    
    // Create window
    window = SDL_CreateWindow("PanelKit",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             640, 480, 0);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return -1;
    }
    
    // Create renderer and texture
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                              SDL_TEXTUREACCESS_STATIC, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    buffer = calloc(LV_HOR_RES_MAX * LV_VER_RES_MAX, sizeof(uint32_t));
    
    if (!renderer || !texture || !buffer) {
        fprintf(stderr, "Renderer/texture/buffer creation failed\n");
        return -1;
    }
#endif

    // Initialize display buffer
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, DISP_BUF_SIZE);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.hor_res = 640;
    disp_drv.ver_res = 480;
    
#if LV_USE_SDL
    disp_drv.flush_cb = sdl_display_flush;
#endif
    
    // Register display driver
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    
    // Initialize input device driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    
#if LV_USE_SDL
    indev_drv.read_cb = sdl_mouse_read;
#endif
    
    // Register input device driver
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv);
    
    // Initialize random number generator for random color button
    srand(time(NULL));
    
    return 0;
}

void platform_deinit(void)
{
#if LV_USE_SDL
    // Clean up SDL resources
    if (buffer) free(buffer);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
#endif
}

int platform_handle_tasks(void)
{
#if LV_USE_SDL
    // Handle SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return 1;  // Exit application
        }
    }
#endif
    
    // Call LVGL tick and task handler
    lv_tick_inc(5);  // 5ms elapsed
    lv_task_handler();
    
    return 0;  // Continue running
}