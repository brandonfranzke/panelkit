// sdl_drm_renderer.h - SDL + Direct DRM integration header
#ifndef SDL_DRM_RENDERER_H
#define SDL_DRM_RENDERER_H

#include <SDL.h>
#include <stdint.h>

typedef struct SDLDRMRenderer SDLDRMRenderer;

// Initialize SDL+DRM renderer
// Returns NULL on failure
SDLDRMRenderer* sdl_drm_init(int width, int height);

// Get SDL window for rendering
SDL_Window* sdl_drm_get_window(SDLDRMRenderer* renderer);

// Get SDL renderer for drawing
SDL_Renderer* sdl_drm_get_renderer(SDLDRMRenderer* renderer);

// Present SDL content to DRM display
// Call this after SDL_RenderPresent()
void sdl_drm_present(SDLDRMRenderer* renderer);

// Cleanup
void sdl_drm_cleanup(SDLDRMRenderer* renderer);

#endif // SDL_DRM_RENDERER_H