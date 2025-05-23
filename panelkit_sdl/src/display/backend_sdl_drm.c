/**
 * @file backend_sdl_drm.c
 * @brief SDL+DRM display backend implementation
 * 
 * This backend uses SDL for software rendering to an offscreen surface,
 * then copies the pixels to a DRM dumb buffer for direct display output.
 * This approach requires only libdrm (~200KB) instead of the full Mesa
 * stack (~169MB).
 */

#include "display_backend.h"
#include "../core/logger.h"

#ifdef __linux__

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

/* DRM buffer structure */
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

/* SDL+DRM backend implementation data */
typedef struct {
    /* DRM resources */
    int drm_fd;
    drmModeConnector* connector;
    drmModeModeInfo* mode;
    uint32_t crtc_id;
    DRMBuffer* buffer;
    
    /* SDL resources */
    bool owns_sdl_init;
} SDLDRMBackendImpl;

/* Create DRM buffer */
static DRMBuffer* create_drm_buffer(int fd, int width, int height) {
    DRMBuffer* buf = calloc(1, sizeof(DRMBuffer));
    if (!buf) {
        log_error("Failed to allocate DRM buffer structure");
        return NULL;
    }
    
    buf->fd = fd;
    buf->width = width;
    buf->height = height;
    
    /* Create dumb buffer */
    struct drm_mode_create_dumb create = {
        .width = width,
        .height = height,
        .bpp = 32
    };
    
    if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        LOG_ERRNO("Failed to create dumb buffer");
        free(buf);
        return NULL;
    }
    
    buf->handle = create.handle;
    buf->pitch = create.pitch;
    buf->size = create.size;
    
    log_debug("Created DRM buffer: %dx%d, pitch=%d, size=%zu",
              width, height, buf->pitch, buf->size);
    
    /* Create framebuffer */
    if (drmModeAddFB(fd, width, height, 24, 32, buf->pitch,
                     buf->handle, &buf->fb_id)) {
        LOG_ERRNO("Failed to create framebuffer");
        struct drm_mode_destroy_dumb destroy = { .handle = buf->handle };
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        free(buf);
        return NULL;
    }
    
    /* Map buffer */
    struct drm_mode_map_dumb map = { .handle = buf->handle };
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map)) {
        LOG_ERRNO("Failed to map dumb buffer");
        drmModeRmFB(fd, buf->fb_id);
        struct drm_mode_destroy_dumb destroy = { .handle = buf->handle };
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        free(buf);
        return NULL;
    }
    
    buf->pixels = mmap(0, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fd, map.offset);
    if (buf->pixels == MAP_FAILED) {
        LOG_ERRNO("Failed to mmap buffer");
        drmModeRmFB(fd, buf->fb_id);
        struct drm_mode_destroy_dumb destroy = { .handle = buf->handle };
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        free(buf);
        return NULL;
    }
    
    /* Clear buffer to black */
    memset(buf->pixels, 0, buf->size);
    
    return buf;
}

/* Destroy DRM buffer */
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

/* Setup DRM display */
static int setup_drm_display(SDLDRMBackendImpl* impl) {
    /* Try vc4 driver first (card1), fallback to card0 */
    const char* drm_devices[] = { "/dev/dri/card1", "/dev/dri/card0", NULL };
    
    for (int i = 0; drm_devices[i]; i++) {
        impl->drm_fd = open(drm_devices[i], O_RDWR);
        if (impl->drm_fd >= 0) {
            log_info("Opened DRM device: %s", drm_devices[i]);
            break;
        }
    }
    
    if (impl->drm_fd < 0) {
        LOG_ERRNO("Failed to open any DRM device");
        return -1;
    }
    
    /* Get resources */
    drmModeRes* res = drmModeGetResources(impl->drm_fd);
    if (!res) {
        log_error("Failed to get DRM resources");
        return -1;
    }
    
    /* Find connected connector */
    impl->connector = NULL;
    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector* conn = drmModeGetConnector(impl->drm_fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            impl->connector = conn;
            impl->mode = &conn->modes[0]; /* Use first (preferred) mode */
            log_info("Found connected display: %dx%d @ %dHz",
                     impl->mode->hdisplay, impl->mode->vdisplay,
                     impl->mode->vrefresh);
            break;
        }
        if (conn) drmModeFreeConnector(conn);
    }
    
    if (!impl->connector) {
        log_error("No connected display found");
        drmModeFreeResources(res);
        return -1;
    }
    
    /* Find CRTC */
    impl->crtc_id = 0;
    
    /* Try to find encoder and associated CRTC */
    if (impl->connector->encoder_id) {
        drmModeEncoder* encoder = drmModeGetEncoder(impl->drm_fd, impl->connector->encoder_id);
        if (encoder) {
            impl->crtc_id = encoder->crtc_id;
            log_debug("Using CRTC %d from current encoder", impl->crtc_id);
            drmModeFreeEncoder(encoder);
        }
    }
    
    /* Fallback to first available CRTC */
    if (!impl->crtc_id && res->count_crtcs > 0) {
        impl->crtc_id = res->crtcs[0];
        log_debug("Using first available CRTC %d", impl->crtc_id);
    }
    
    if (!impl->crtc_id) {
        log_error("No CRTC available");
        drmModeFreeConnector(impl->connector);
        drmModeFreeResources(res);
        return -1;
    }
    
    drmModeFreeResources(res);
    return 0;
}

/* Present function - copy SDL surface to DRM buffer */
static void sdl_drm_backend_present(DisplayBackend* backend) {
    if (!backend || !backend->impl) {
        return;
    }
    
    SDLDRMBackendImpl* impl = (SDLDRMBackendImpl*)backend->impl;
    if (!impl->buffer) {
        return;
    }
    
    /* Get SDL surface */
    SDL_Surface* surface = SDL_GetWindowSurface(backend->window);
    if (!surface) {
        log_error("Failed to get SDL window surface");
        return;
    }
    
    SDL_LockSurface(surface);
    
    /* Copy SDL surface to DRM buffer */
    int min_height = surface->h < impl->buffer->height ? surface->h : impl->buffer->height;
    int min_width = surface->w < impl->buffer->width ? surface->w : impl->buffer->width;
    
    for (int y = 0; y < min_height; y++) {
        uint32_t* src = (uint32_t*)((uint8_t*)surface->pixels + y * surface->pitch);
        uint32_t* dst = impl->buffer->pixels + y * (impl->buffer->pitch / 4);
        memcpy(dst, src, min_width * 4);
    }
    
    SDL_UnlockSurface(surface);
    
    /* Update display by setting CRTC again - simple but works */
    /* Note: A proper implementation would use page flipping, but this is simpler */
    if (drmModeSetCrtc(impl->drm_fd, impl->crtc_id, impl->buffer->fb_id, 0, 0,
                       &impl->connector->connector_id, 1, impl->mode)) {
        /* Ignore errors - display might still work */
    }
}

/* Cleanup function */
static void sdl_drm_backend_cleanup(DisplayBackend* backend) {
    if (!backend || !backend->impl) {
        return;
    }
    
    SDLDRMBackendImpl* impl = (SDLDRMBackendImpl*)backend->impl;
    
    /* Destroy DRM resources */
    destroy_drm_buffer(impl->buffer);
    if (impl->connector) drmModeFreeConnector(impl->connector);
    if (impl->drm_fd >= 0) close(impl->drm_fd);
    
    /* Destroy SDL resources */
    if (backend->renderer) {
        SDL_DestroyRenderer(backend->renderer);
        backend->renderer = NULL;
    }
    
    if (backend->window) {
        SDL_DestroyWindow(backend->window);
        backend->window = NULL;
    }
    
    /* Quit SDL if we initialized it */
    if (impl->owns_sdl_init) {
        SDL_Quit();
    }
    
    free(impl);
    backend->impl = NULL;
}

/* Create SDL+DRM backend */
DisplayBackend* display_backend_sdl_drm_create(const DisplayConfig* config) {
    /* Allocate backend structure */
    DisplayBackend* backend = calloc(1, sizeof(DisplayBackend));
    if (!backend) {
        log_error("Failed to allocate display backend");
        return NULL;
    }
    
    /* Allocate implementation data */
    SDLDRMBackendImpl* impl = calloc(1, sizeof(SDLDRMBackendImpl));
    if (!impl) {
        log_error("Failed to allocate SDL+DRM backend implementation");
        free(backend);
        return NULL;
    }
    
    /* Set up backend structure */
    backend->type = DISPLAY_BACKEND_SDL_DRM;
    backend->name = "SDL+DRM";
    backend->impl = impl;
    backend->present = sdl_drm_backend_present;
    backend->cleanup = sdl_drm_backend_cleanup;
    
    /* Setup DRM first to get actual display resolution */
    if (setup_drm_display(impl) < 0) {
        free(impl);
        free(backend);
        return NULL;
    }
    
    /* Use actual display dimensions */
    backend->actual_width = impl->mode->hdisplay;
    backend->actual_height = impl->mode->vdisplay;
    
    log_info("Using display mode: %dx%d", backend->actual_width, backend->actual_height);
    
    /* Initialize SDL with offscreen driver */
    /* Set environment variable as well for older SDL versions */
    setenv("SDL_VIDEODRIVER", "offscreen", 1);
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    
    /* List available video drivers for debugging */
    int num_drivers = SDL_GetNumVideoDrivers();
    log_info("Available SDL video drivers: %d", num_drivers);
    for (int i = 0; i < num_drivers; i++) {
        log_info("  Driver %d: %s", i, SDL_GetVideoDriver(i));
    }
    
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            LOG_SDL_ERROR("Failed to initialize SDL with offscreen driver");
            /* Try dummy driver as fallback */
            setenv("SDL_VIDEODRIVER", "dummy", 1);
            SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                LOG_SDL_ERROR("Failed to initialize SDL with dummy driver");
                /* Try without any driver hint */
                unsetenv("SDL_VIDEODRIVER");
                if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
                    LOG_SDL_ERROR("Failed to initialize SDL video subsystem");
                    sdl_drm_backend_cleanup(backend);
                    free(backend);
                    return NULL;
                }
                log_info("SDL video subsystem initialized without driver hint");
            } else {
                log_info("SDL video subsystem initialized with dummy driver");
            }
        } else {
            log_info("SDL video subsystem initialized with offscreen driver");
        }
        impl->owns_sdl_init = true;
    }
    
    /* Create window (offscreen) */
    backend->window = SDL_CreateWindow(
        config->title ? config->title : "PanelKit",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        backend->actual_width,
        backend->actual_height,
        0
    );
    
    if (!backend->window) {
        LOG_SDL_ERROR("Failed to create offscreen window");
        sdl_drm_backend_cleanup(backend);
        free(backend);
        return NULL;
    }
    
    /* Create renderer (software) */
    backend->renderer = SDL_CreateRenderer(backend->window, -1, SDL_RENDERER_SOFTWARE);
    if (!backend->renderer) {
        LOG_SDL_ERROR("Failed to create software renderer");
        sdl_drm_backend_cleanup(backend);
        free(backend);
        return NULL;
    }
    
    /* Create DRM buffer */
    impl->buffer = create_drm_buffer(impl->drm_fd, 
                                     impl->mode->hdisplay, 
                                     impl->mode->vdisplay);
    if (!impl->buffer) {
        sdl_drm_backend_cleanup(backend);
        free(backend);
        return NULL;
    }
    
    /* Set display mode */
    if (drmModeSetCrtc(impl->drm_fd, impl->crtc_id, impl->buffer->fb_id, 0, 0,
                       &impl->connector->connector_id, 1, impl->mode)) {
        LOG_ERRNO("Failed to set CRTC");
        /* Continue anyway - might already be set correctly */
    }
    
    log_info("SDL+DRM backend initialized successfully");
    
    return backend;
}

#else /* !__linux__ */

/* Stub implementation for non-Linux platforms */
DisplayBackend* display_backend_sdl_drm_create(const DisplayConfig* config) {
    (void)config;
    log_error("SDL+DRM backend is only available on Linux");
    return NULL;
}

#endif /* __linux__ */