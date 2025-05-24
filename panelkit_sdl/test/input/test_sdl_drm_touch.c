/**
 * test_sdl_drm_touch.c - Combined SDL+DRM display with manual touch input
 * 
 * This test combines:
 * - SDL rendering with offscreen driver (no libgbm dependency)
 * - Direct DRM output for display
 * - Manual touch input reading from /dev/input/event*
 * - Touch event injection into SDL
 */

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>

// Global flag for clean shutdown
static volatile int keep_running = 1;

// Touch device info
static int touch_fd = -1;
static int screen_width = 0;
static int screen_height = 0;

// Helper macros for input handling
#define NBITS(x) (((x)/BITS_PER_LONG)+1)
#define BITS_PER_LONG (sizeof(long)*8)
#define test_bit(bit, array) ((array[(bit)/BITS_PER_LONG] >> ((bit)%BITS_PER_LONG)) & 1)

// DRM buffer structure
typedef struct {
    int fd;
    uint32_t* pixels;
    size_t size;
    uint32_t handle;
    uint32_t pitch;
    uint32_t fb_id;
    int width;
    int height;
} DRMBuffer;

// DRM state
typedef struct {
    int fd;
    drmModeConnector* connector;
    drmModeModeInfo* mode;
    uint32_t crtc_id;
    DRMBuffer* buffer;
} DRMState;

void signal_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

// Find touch device by looking for devices with ABS_MT_POSITION_X
int find_touch_device(char* path, size_t path_size) {
    DIR* dir = opendir("/dev/input");
    if (!dir) return -1;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;
        
        snprintf(path, path_size, "/dev/input/%s", entry->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        
        // Check if device has touch capabilities
        unsigned long evbit[NBITS(EV_MAX)];
        unsigned long absbit[NBITS(ABS_MAX)];
        memset(evbit, 0, sizeof(evbit));
        memset(absbit, 0, sizeof(absbit));
        
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) >= 0 &&
            ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) >= 0) {
            
            if (test_bit(EV_ABS, evbit) && 
                test_bit(ABS_MT_POSITION_X, absbit) &&
                test_bit(ABS_MT_POSITION_Y, absbit)) {
                close(fd);
                closedir(dir);
                return 0; // Found touch device
            }
        }
        close(fd);
    }
    
    closedir(dir);
    return -1; // No touch device found
}

// Touch input thread - reads from Linux input and injects into SDL
void* touch_input_thread(void* arg) {
    (void)arg;
    char device_path[256];
    struct input_event ev;
    
    // Find touch device
    if (find_touch_device(device_path, sizeof(device_path)) < 0) {
        printf("No touch device found\n");
        return NULL;
    }
    
    printf("Found touch device: %s\n", device_path);
    
    // Open touch device
    touch_fd = open(device_path, O_RDONLY);
    if (touch_fd < 0) {
        perror("Failed to open touch device");
        return NULL;
    }
    
    // Get device info for coordinate scaling
    struct input_absinfo abs_x, abs_y;
    if (ioctl(touch_fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_x) < 0 ||
        ioctl(touch_fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_y) < 0) {
        printf("Failed to get touch device info\n");
        close(touch_fd);
        return NULL;
    }
    
    printf("Touch device range: X=%d-%d, Y=%d-%d\n", 
           abs_x.minimum, abs_x.maximum, abs_y.minimum, abs_y.maximum);
    
    // Touch state tracking
    int touch_x = 0, touch_y = 0;
    int touch_active = 0;
    int tracking_id = -1;
    
    while (keep_running) {
        if (read(touch_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_ABS) {
                switch (ev.code) {
                    case ABS_MT_POSITION_X:
                        touch_x = ev.value;
                        break;
                        
                    case ABS_MT_POSITION_Y:
                        touch_y = ev.value;
                        break;
                        
                    case ABS_MT_TRACKING_ID:
                        if (ev.value == -1 && tracking_id != -1) {
                            // Touch up
                            SDL_Event sdl_ev;
                            sdl_ev.type = SDL_FINGERUP;
                            sdl_ev.tfinger.touchId = 0;
                            sdl_ev.tfinger.fingerId = tracking_id;
                            sdl_ev.tfinger.x = (float)(touch_x - abs_x.minimum) / 
                                             (abs_x.maximum - abs_x.minimum);
                            sdl_ev.tfinger.y = (float)(touch_y - abs_y.minimum) / 
                                             (abs_y.maximum - abs_y.minimum);
                            sdl_ev.tfinger.pressure = 0.0f;
                            SDL_PushEvent(&sdl_ev);
                            
                            printf("Touch UP at %d,%d (normalized: %.3f,%.3f)\n", 
                                   touch_x, touch_y, sdl_ev.tfinger.x, sdl_ev.tfinger.y);
                            
                            touch_active = 0;
                            tracking_id = -1;
                        } else if (ev.value >= 0) {
                            // Touch down
                            tracking_id = ev.value;
                            SDL_Event sdl_ev;
                            sdl_ev.type = SDL_FINGERDOWN;
                            sdl_ev.tfinger.touchId = 0;
                            sdl_ev.tfinger.fingerId = tracking_id;
                            sdl_ev.tfinger.x = (float)(touch_x - abs_x.minimum) / 
                                             (abs_x.maximum - abs_x.minimum);
                            sdl_ev.tfinger.y = (float)(touch_y - abs_y.minimum) / 
                                             (abs_y.maximum - abs_y.minimum);
                            sdl_ev.tfinger.pressure = 1.0f;
                            SDL_PushEvent(&sdl_ev);
                            
                            printf("Touch DOWN at %d,%d (normalized: %.3f,%.3f)\n", 
                                   touch_x, touch_y, sdl_ev.tfinger.x, sdl_ev.tfinger.y);
                            
                            touch_active = 1;
                        }
                        break;
                }
            } else if (ev.type == EV_SYN && ev.code == SYN_REPORT && touch_active) {
                // Touch motion
                SDL_Event sdl_ev;
                sdl_ev.type = SDL_FINGERMOTION;
                sdl_ev.tfinger.touchId = 0;
                sdl_ev.tfinger.fingerId = tracking_id;
                sdl_ev.tfinger.x = (float)(touch_x - abs_x.minimum) / 
                                 (abs_x.maximum - abs_x.minimum);
                sdl_ev.tfinger.y = (float)(touch_y - abs_y.minimum) / 
                                 (abs_y.maximum - abs_y.minimum);
                sdl_ev.tfinger.dx = 0; // Would need to track previous position
                sdl_ev.tfinger.dy = 0;
                sdl_ev.tfinger.pressure = 1.0f;
                SDL_PushEvent(&sdl_ev);
            }
        }
        
        usleep(1000); // 1ms sleep to prevent CPU spinning
    }
    
    close(touch_fd);
    return NULL;
}

// DRM buffer creation
DRMBuffer* drm_create_buffer(int fd, int width, int height) {
    DRMBuffer* buf = calloc(1, sizeof(DRMBuffer));
    if (!buf) return NULL;
    
    buf->fd = fd;
    buf->width = width;
    buf->height = height;
    
    // Create dumb buffer
    struct drm_mode_create_dumb create = {
        .width = width,
        .height = height,
        .bpp = 32
    };
    
    if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        free(buf);
        return NULL;
    }
    
    buf->handle = create.handle;
    buf->size = create.size;
    buf->pitch = create.pitch;
    
    // Create framebuffer
    if (drmModeAddFB(fd, width, height, 24, 32, create.pitch,
                     create.handle, &buf->fb_id)) {
        struct drm_mode_destroy_dumb destroy = { .handle = create.handle };
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        free(buf);
        return NULL;
    }
    
    // Map buffer
    struct drm_mode_map_dumb map = { .handle = create.handle };
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
        drmModeRmFB(fd, buf->fb_id);
        struct drm_mode_destroy_dumb destroy = { .handle = create.handle };
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        free(buf);
        return NULL;
    }
    
    buf->pixels = mmap(0, create.size, PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd, map.offset);
    
    if (buf->pixels == MAP_FAILED) {
        drmModeRmFB(fd, buf->fb_id);
        struct drm_mode_destroy_dumb destroy = { .handle = create.handle };
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        free(buf);
        return NULL;
    }
    
    // Clear to black
    memset(buf->pixels, 0, buf->size);
    
    return buf;
}

// DRM initialization
DRMState* drm_init() {
    DRMState* state = calloc(1, sizeof(DRMState));
    if (!state) return NULL;
    
    // Try card1 first (usually vc4 on Pi)
    state->fd = open("/dev/dri/card1", O_RDWR);
    if (state->fd < 0) {
        state->fd = open("/dev/dri/card0", O_RDWR);
        if (state->fd < 0) {
            free(state);
            return NULL;
        }
    }
    
    // Get resources
    drmModeRes* res = drmModeGetResources(state->fd);
    if (!res) {
        close(state->fd);
        free(state);
        return NULL;
    }
    
    // Find connected display
    for (int i = 0; i < res->count_connectors; i++) {
        state->connector = drmModeGetConnector(state->fd, res->connectors[i]);
        if (state->connector && state->connector->connection == DRM_MODE_CONNECTED) {
            break;
        }
        if (state->connector) {
            drmModeFreeConnector(state->connector);
            state->connector = NULL;
        }
    }
    
    if (!state->connector) {
        drmModeFreeResources(res);
        close(state->fd);
        free(state);
        return NULL;
    }
    
    // Get display mode
    state->mode = &state->connector->modes[0];
    screen_width = state->mode->hdisplay;
    screen_height = state->mode->vdisplay;
    
    printf("Display: %dx%d\n", screen_width, screen_height);
    
    // Find CRTC
    if (state->connector->encoder_id) {
        drmModeEncoder* encoder = drmModeGetEncoder(state->fd, state->connector->encoder_id);
        if (encoder) {
            state->crtc_id = encoder->crtc_id;
            drmModeFreeEncoder(encoder);
        }
    }
    
    if (!state->crtc_id && res->count_crtcs > 0) {
        state->crtc_id = res->crtcs[0];
    }
    
    drmModeFreeResources(res);
    
    // Create buffer
    state->buffer = drm_create_buffer(state->fd, screen_width, screen_height);
    if (!state->buffer) {
        drmModeFreeConnector(state->connector);
        close(state->fd);
        free(state);
        return NULL;
    }
    
    // Set display mode
    if (drmModeSetCrtc(state->fd, state->crtc_id, state->buffer->fb_id, 0, 0,
                       &state->connector->connector_id, 1, state->mode)) {
        printf("Warning: Failed to set CRTC\n");
    }
    
    return state;
}

// Copy SDL surface to DRM buffer
void copy_sdl_to_drm(SDL_Surface* surface, DRMBuffer* drm) {
    SDL_LockSurface(surface);
    
    int min_h = surface->h < drm->height ? surface->h : drm->height;
    int min_w = surface->w < drm->width ? surface->w : drm->width;
    
    for (int y = 0; y < min_h; y++) {
        uint32_t* src = (uint32_t*)((uint8_t*)surface->pixels + y * surface->pitch);
        uint32_t* dst = drm->pixels + y * (drm->pitch / 4);
        memcpy(dst, src, min_w * 4);
    }
    
    SDL_UnlockSurface(surface);
}

int main() {
    printf("=== SDL+DRM with Touch Input Test ===\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize DRM
    DRMState* drm = drm_init();
    if (!drm) {
        printf("Failed to initialize DRM\n");
        return 1;
    }
    
    // Initialize SDL with offscreen driver
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create SDL window matching display
    SDL_Window* window = SDL_CreateWindow("Touch Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screen_width, screen_height, 0);
    
    if (!window) {
        printf("Failed to create window\n");
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Failed to create renderer\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Start touch input thread
    pthread_t touch_thread;
    if (pthread_create(&touch_thread, NULL, touch_input_thread, NULL) != 0) {
        printf("Failed to create touch thread\n");
        return 1;
    }
    
    printf("\nTouch the screen to test input!\n");
    printf("Yellow squares show touch points\n");
    printf("Press Ctrl+C to exit\n\n");
    
    // Main loop
    SDL_Event event;
    int touch_count = 0;
    
    // Track active touches for visualization
    typedef struct {
        int active;
        float x, y;
    } TouchPoint;
    TouchPoint touches[10] = {0};
    
    while (keep_running) {
        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_FINGERDOWN:
                    touch_count++;
                    printf("[SDL] Touch %d DOWN at %.3f,%.3f\n", 
                           touch_count, event.tfinger.x, event.tfinger.y);
                    
                    // Store touch point
                    if (event.tfinger.fingerId < 10) {
                        touches[event.tfinger.fingerId].active = 1;
                        touches[event.tfinger.fingerId].x = event.tfinger.x;
                        touches[event.tfinger.fingerId].y = event.tfinger.y;
                    }
                    break;
                    
                case SDL_FINGERUP:
                    printf("[SDL] Touch UP\n");
                    
                    // Clear touch point
                    if (event.tfinger.fingerId < 10) {
                        touches[event.tfinger.fingerId].active = 0;
                    }
                    break;
                    
                case SDL_FINGERMOTION:
                    // Update touch point
                    if (event.tfinger.fingerId < 10) {
                        touches[event.tfinger.fingerId].x = event.tfinger.x;
                        touches[event.tfinger.fingerId].y = event.tfinger.y;
                    }
                    break;
                    
                case SDL_QUIT:
                    keep_running = 0;
                    break;
            }
        }
        
        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255); // Dark blue background
        SDL_RenderClear(renderer);
        
        // Draw touch points
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
        for (int i = 0; i < 10; i++) {
            if (touches[i].active) {
                SDL_Rect rect = {
                    (int)(touches[i].x * screen_width) - 20,
                    (int)(touches[i].y * screen_height) - 20,
                    40, 40
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        
        // Info text area
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_Rect info_bg = {10, 10, 300, 60};
        SDL_RenderFillRect(renderer, &info_bg);
        
        SDL_RenderPresent(renderer);
        
        // Copy to DRM
        SDL_Surface* surface = SDL_GetWindowSurface(window);
        if (surface) {
            copy_sdl_to_drm(surface, drm->buffer);
        }
        
        SDL_Delay(16); // ~60 FPS
    }
    
    printf("\nShutting down...\n");
    printf("Total touches: %d\n", touch_count);
    
    // Wait for touch thread
    pthread_join(touch_thread, NULL);
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}