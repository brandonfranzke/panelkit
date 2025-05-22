// sdl_drm_renderer.c - SDL + Direct DRM integration implementation
#include "sdl_drm_renderer.h"
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

struct SDLDRMRenderer {
    // SDL components
    SDL_Window* window;
    SDL_Renderer* renderer;
    
    // DRM components
    int drm_fd;
    drmModeConnector* connector;
    drmModeModeInfo* mode;
    uint32_t crtc_id;
    DRMBuffer* buffer;
};

static DRMBuffer* create_drm_buffer(int fd, int width, int height) {
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
    buf->pitch = create.pitch;
    buf->size = create.size;
    
    // Create framebuffer
    if (drmModeAddFB(fd, width, height, 24, 32, buf->pitch,
                     buf->handle, &buf->fb_id)) {
        printf("Failed to create framebuffer: %s\n", strerror(errno));
        free(buf);
        return NULL;
    }
    
    // Map buffer
    struct drm_mode_map_dumb map = { .handle = buf->handle };
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map)) {
        printf("Failed to map dumb buffer: %s\n", strerror(errno));
        drmModeRmFB(fd, buf->fb_id);
        free(buf);
        return NULL;
    }
    
    buf->pixels = mmap(0, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fd, map.offset);
    if (buf->pixels == MAP_FAILED) {
        printf("Failed to mmap buffer: %s\n", strerror(errno));
        drmModeRmFB(fd, buf->fb_id);
        free(buf);
        return NULL;
    }
    
    return buf;
}

static void destroy_drm_buffer(DRMBuffer* buf) {
    if (!buf) return;
    
    if (buf->pixels != MAP_FAILED) {
        munmap(buf->pixels, buf->size);
    }
    if (buf->fb_id) {
        drmModeRmFB(buf->fd, buf->fb_id);
    }
    
    struct drm_mode_destroy_dumb destroy = { .handle = buf->handle };
    ioctl(buf->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    
    free(buf);
}

static int setup_drm_display(SDLDRMRenderer* ctx) {
    // Try vc4 driver first (card1), fallback to card0
    ctx->drm_fd = open("/dev/dri/card1", O_RDWR);
    if (ctx->drm_fd < 0) {
        ctx->drm_fd = open("/dev/dri/card0", O_RDWR);
        if (ctx->drm_fd < 0) {
            printf("Failed to open DRM device: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Get resources
    drmModeRes* res = drmModeGetResources(ctx->drm_fd);
    if (!res) {
        printf("Failed to get DRM resources\n");
        return -1;
    }
    
    // Find connected connector
    ctx->connector = NULL;
    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector* conn = drmModeGetConnector(ctx->drm_fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            ctx->connector = conn;
            ctx->mode = &conn->modes[0]; // Use first (preferred) mode
            break;
        }
        if (conn) drmModeFreeConnector(conn);
    }
    
    if (!ctx->connector) {
        printf("No connected display found\n");
        drmModeFreeResources(res);
        return -1;
    }
    
    // Find CRTC
    if (res->count_crtcs > 0) {
        ctx->crtc_id = res->crtcs[0];
    } else {
        printf("No CRTC available\n");
        drmModeFreeConnector(ctx->connector);
        drmModeFreeResources(res);
        return -1;
    }
    
    drmModeFreeResources(res);
    return 0;
}

SDLDRMRenderer* sdl_drm_init(int width, int height) {
    SDLDRMRenderer* ctx = calloc(1, sizeof(SDLDRMRenderer));
    if (!ctx) return NULL;
    
    // Initialize SDL with offscreen driver
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        free(ctx);
        return NULL;
    }
    
    // Create window and renderer
    ctx->window = SDL_CreateWindow("SDL+DRM",
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   width, height, 0);
    if (!ctx->window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        free(ctx);
        return NULL;
    }
    
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_SOFTWARE);
    if (!ctx->renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        free(ctx);
        return NULL;
    }
    
    // Setup DRM
    if (setup_drm_display(ctx) < 0) {
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        free(ctx);
        return NULL;
    }
    
    // Create DRM buffer
    ctx->buffer = create_drm_buffer(ctx->drm_fd, ctx->mode->hdisplay, ctx->mode->vdisplay);
    if (!ctx->buffer) {
        drmModeFreeConnector(ctx->connector);
        close(ctx->drm_fd);
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        free(ctx);
        return NULL;
    }
    
    // Set display mode
    if (drmModeSetCrtc(ctx->drm_fd, ctx->crtc_id, ctx->buffer->fb_id, 0, 0,
                       &ctx->connector->connector_id, 1, ctx->mode)) {
        printf("Failed to set CRTC: %s\n", strerror(errno));
    }
    
    return ctx;
}

SDL_Window* sdl_drm_get_window(SDLDRMRenderer* renderer) {
    return renderer ? renderer->window : NULL;
}

SDL_Renderer* sdl_drm_get_renderer(SDLDRMRenderer* renderer) {
    return renderer ? renderer->renderer : NULL;
}

void sdl_drm_present(SDLDRMRenderer* ctx) {
    if (!ctx || !ctx->buffer) return;
    
    // Get SDL surface
    SDL_Surface* surface = SDL_GetWindowSurface(ctx->window);
    if (!surface) return;
    
    SDL_LockSurface(surface);
    
    // Copy SDL surface to DRM buffer
    int min_height = surface->h < ctx->buffer->height ? surface->h : ctx->buffer->height;
    int min_width = surface->w < ctx->buffer->width ? surface->w : ctx->buffer->width;
    
    for (int y = 0; y < min_height; y++) {
        uint32_t* src = (uint32_t*)((uint8_t*)surface->pixels + y * surface->pitch);
        uint32_t* dst = ctx->buffer->pixels + y * (ctx->buffer->pitch / 4);
        memcpy(dst, src, min_width * 4);
    }
    
    SDL_UnlockSurface(surface);
}

void sdl_drm_cleanup(SDLDRMRenderer* ctx) {
    if (!ctx) return;
    
    destroy_drm_buffer(ctx->buffer);
    if (ctx->connector) drmModeFreeConnector(ctx->connector);
    if (ctx->drm_fd >= 0) close(ctx->drm_fd);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    SDL_Quit();
    free(ctx);
}