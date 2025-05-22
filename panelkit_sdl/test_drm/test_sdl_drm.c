// test_sdl_drm.c - SDL with direct DRM output
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>

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

typedef struct {
    int fd;
    drmModeConnector* connector;
    drmModeModeInfo* mode;
    uint32_t crtc_id;
    DRMBuffer* buffer;
} DRMState;

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
        printf("Failed to create dumb buffer: %s\n", strerror(errno));
        free(buf);
        return NULL;
    }
    
    buf->handle = create.handle;
    buf->size = create.size;
    buf->pitch = create.pitch;
    
    // Create framebuffer object
    if (drmModeAddFB(fd, width, height, 24, 32, create.pitch,
                     create.handle, &buf->fb_id)) {
        printf("Failed to create framebuffer: %s\n", strerror(errno));
        free(buf);
        return NULL;
    }
    
    // Map it
    struct drm_mode_map_dumb map = { .handle = create.handle };
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
        printf("Failed to map buffer: %s\n", strerror(errno));
        drmModeRmFB(fd, buf->fb_id);
        free(buf);
        return NULL;
    }
    
    buf->pixels = mmap(0, create.size, PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd, map.offset);
    
    if (buf->pixels == MAP_FAILED) {
        printf("Failed to mmap: %s\n", strerror(errno));
        drmModeRmFB(fd, buf->fb_id);
        free(buf);
        return NULL;
    }
    
    return buf;
}

void drm_destroy_buffer(DRMBuffer* buf) {
    if (!buf) return;
    
    munmap(buf->pixels, buf->size);
    drmModeRmFB(buf->fd, buf->fb_id);
    
    struct drm_mode_destroy_dumb destroy = { .handle = buf->handle };
    ioctl(buf->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    
    free(buf);
}

DRMState* drm_init() {
    DRMState* state = calloc(1, sizeof(DRMState));
    if (!state) return NULL;
    
    // Try to open DRM devices - start with card1 (vc4) which supports dumb buffers
    state->fd = open("/dev/dri/card1", O_RDWR);
    if (state->fd < 0) {
        // Fall back to card0
        state->fd = open("/dev/dri/card0", O_RDWR);
        if (state->fd < 0) {
            printf("Failed to open any DRM device: %s\n", strerror(errno));
            free(state);
            return NULL;
        }
        printf("Using /dev/dri/card0\n");
    } else {
        printf("Using /dev/dri/card1\n");
    }
    
    // Get resources
    drmModeRes* res = drmModeGetResources(state->fd);
    if (!res) {
        printf("Failed to get DRM resources\n");
        close(state->fd);
        free(state);
        return NULL;
    }
    
    // Find a connected connector
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
        printf("No connected display found\n");
        drmModeFreeResources(res);
        close(state->fd);
        free(state);
        return NULL;
    }
    
    // Get mode
    state->mode = &state->connector->modes[0];
    printf("Using mode: %dx%d\n", state->mode->hdisplay, state->mode->vdisplay);
    
    // Find encoder and CRTC
    drmModeEncoder* encoder = drmModeGetEncoder(state->fd, state->connector->encoder_id);
    if (encoder) {
        state->crtc_id = encoder->crtc_id;
        drmModeFreeEncoder(encoder);
    } else {
        // Find a suitable CRTC
        for (int i = 0; i < res->count_crtcs; i++) {
            state->crtc_id = res->crtcs[i];
            break;
        }
    }
    
    drmModeFreeResources(res);
    
    // Create buffer
    state->buffer = drm_create_buffer(state->fd, state->mode->hdisplay, state->mode->vdisplay);
    if (!state->buffer) {
        printf("Failed to create buffer\n");
        drmModeFreeConnector(state->connector);
        close(state->fd);
        free(state);
        return NULL;
    }
    
    return state;
}

void drm_cleanup(DRMState* state) {
    if (!state) return;
    
    drm_destroy_buffer(state->buffer);
    drmModeFreeConnector(state->connector);
    close(state->fd);
    free(state);
}

void drm_copy_from_sdl(DRMBuffer* drm, SDL_Surface* surface) {
    SDL_LockSurface(surface);
    
    int min_height = surface->h < drm->height ? surface->h : drm->height;
    int min_width = surface->w < drm->width ? surface->w : drm->width;
    
    for (int y = 0; y < min_height; y++) {
        uint32_t* src = (uint32_t*)((uint8_t*)surface->pixels + y * surface->pitch);
        uint32_t* dst = drm->pixels + y * (drm->pitch / 4);
        memcpy(dst, src, min_width * 4);
    }
    
    SDL_UnlockSurface(surface);
}

int main(int argc, char* argv[]) {
    printf("=== SDL + Direct DRM Test ===\n\n");
    
    // Initialize DRM
    DRMState* drm = drm_init();
    if (!drm) {
        printf("Failed to initialize DRM\n");
        printf("Make sure:\n");
        printf("  - You have permission to access DRM devices\n");
        printf("  - A display is connected\n");
        printf("  - You're in the 'video' group or running as root\n");
        return 1;
    }
    
    // Initialize SDL with offscreen driver
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        drm_cleanup(drm);
        return 1;
    }
    
    // Create SDL window matching display size
    SDL_Window* window = SDL_CreateWindow("SDL DRM Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        drm->mode->hdisplay, drm->mode->vdisplay, 0);
    
    if (!window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        drm_cleanup(drm);
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        drm_cleanup(drm);
        return 1;
    }
    
    printf("SDL initialized, starting animation...\n");
    printf("Press Ctrl+C to exit\n");
    
    // Set mode
    if (drmModeSetCrtc(drm->fd, drm->crtc_id, drm->buffer->fb_id, 0, 0,
                       &drm->connector->connector_id, 1, drm->mode)) {
        printf("Failed to set CRTC: %s\n", strerror(errno));
    }
    
    // Animation loop
    SDL_Event event;
    int running = 1;
    int frame = 0;
    
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        
        // Render with SDL
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Animated red square
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect rect = {
            100 + (frame % 400),
            100,
            200, 200
        };
        SDL_RenderFillRect(renderer, &rect);
        
        // Green square
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect rect2 = {300, 300, 150, 150};
        SDL_RenderFillRect(renderer, &rect2);
        
        SDL_RenderPresent(renderer);
        
        // Copy to DRM buffer
        SDL_Surface* surface = SDL_GetWindowSurface(window);
        if (surface) {
            drm_copy_from_sdl(drm->buffer, surface);
        }
        
        frame++;
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    drm_cleanup(drm);
    
    printf("\nClean exit\n");
    return 0;
}