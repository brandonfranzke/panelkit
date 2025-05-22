// example_usage.c - Clean example of SDL+DRM usage
#include "sdl_drm_renderer.h"
#include <stdio.h>

int main() {
    printf("=== SDL+DRM Clean Example ===\n");
    
    // Initialize SDL+DRM renderer
    SDLDRMRenderer* renderer = sdl_drm_init(800, 600);
    if (!renderer) {
        printf("Failed to initialize SDL+DRM renderer\n");
        return 1;
    }
    
    SDL_Window* window = sdl_drm_get_window(renderer);
    SDL_Renderer* sdl_renderer = sdl_drm_get_renderer(renderer);
    
    printf("Displaying animated graphics for 5 seconds...\n");
    
    // Simple animation loop
    SDL_Event event;
    int running = 1;
    int frame = 0;
    
    while (running && frame < 300) { // 5 seconds at 60fps
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
        SDL_RenderClear(sdl_renderer);
        
        // Draw animated red rectangle
        SDL_SetRenderDrawColor(sdl_renderer, 255, 0, 0, 255);
        SDL_Rect rect = {
            100 + (frame % 400),  // Moving horizontally
            200,
            100, 100
        };
        SDL_RenderFillRect(sdl_renderer, &rect);
        
        // Draw static green circle (approximated with rects)
        SDL_SetRenderDrawColor(sdl_renderer, 0, 255, 0, 255);
        for (int y = -30; y < 30; y++) {
            for (int x = -30; x < 30; x++) {
                if (x*x + y*y < 900) { // Circle equation
                    SDL_Rect pixel = {400 + x, 300 + y, 1, 1};
                    SDL_RenderFillRect(sdl_renderer, &pixel);
                }
            }
        }
        
        // Present SDL content
        SDL_RenderPresent(sdl_renderer);
        
        // Copy to DRM display
        sdl_drm_present(renderer);
        
        frame++;
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    sdl_drm_cleanup(renderer);
    
    printf("Demo complete!\n");
    return 0;
}