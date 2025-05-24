#include "rendering.h"
#include "../core/logger.h"
#include <math.h>
#include <string.h>

// Rendering context
static SDL_Renderer* g_renderer = NULL;
static TTF_Font* g_font = NULL;
static TTF_Font* g_large_font = NULL;
static TTF_Font* g_small_font = NULL;

// Screen dimensions
static int actual_width = 640;
static int actual_height = 480;

// Page indicator constants - exact values from app.c
#define PAGE_INDICATOR_RADIUS 4
#define PAGE_INDICATOR_SPACING 12  // Reduced from 16 for tighter spacing
#define PAGE_INDICATOR_Y_OFFSET 20  // Distance from bottom
#define PAGE_INDICATOR_CONTAINER_PADDING 10  // Reduced from 12
#define PAGE_INDICATOR_CONTAINER_HEIGHT 20 // Reduced from 24 for better proportions

// Helper macros for dynamic dimensions
#define PAGE_INDICATOR_Y (actual_height - PAGE_INDICATOR_Y_OFFSET)

// Initialize rendering system
void rendering_init(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* large_font, TTF_Font* small_font) {
    g_renderer = renderer;
    g_font = font;
    g_large_font = large_font;
    g_small_font = small_font;
}

// Set screen dimensions
void rendering_set_dimensions(int width, int height) {
    actual_width = width;
    actual_height = height;
}

// Render a button with text and state effects - exact implementation from app.c
void render_button(int x, int y, int w, int h, const char* text, SDL_Color color, ButtonState state) {
    // Adjust colors based on state
    SDL_Color button_color = color;
    
    // Brighten when hovered
    if (state == BUTTON_HOVER) {
        button_color.r = (Uint8)fmin(color.r * 1.2, 255);
        button_color.g = (Uint8)fmin(color.g * 1.2, 255);
        button_color.b = (Uint8)fmin(color.b * 1.2, 255);
    }
    
    // Darken when pressed
    if (state == BUTTON_PRESSED) {
        button_color.r = (Uint8)(color.r * 0.8);
        button_color.g = (Uint8)(color.g * 0.8);
        button_color.b = (Uint8)(color.b * 0.8);
    }
    
    // Pulse when held
    if (state == BUTTON_HELD) {
        Uint32 pulse = (SDL_GetTicks() / 100) % 10;
        float pulse_factor = 0.8f + (pulse / 10.0f) * 0.4f;
        button_color.r = (Uint8)fmin(color.r * pulse_factor, 255);
        button_color.g = (Uint8)fmin(color.g * pulse_factor, 255);
        button_color.b = (Uint8)fmin(color.b * pulse_factor, 255);
    }
    
    // Draw button background
    SDL_Rect button_rect = {x, y, w, h};
    SDL_SetRenderDrawColor(g_renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    SDL_RenderFillRect(g_renderer, &button_rect);
    
    // Draw button border
    if (state != BUTTON_NORMAL) {
        // Thicker border for interactive states
        SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
        SDL_Rect outer_border = {x - 2, y - 2, w + 4, h + 4};
        SDL_RenderDrawRect(g_renderer, &outer_border);
    }
    
    // Normal border
    SDL_SetRenderDrawColor(g_renderer, 
                          button_color.r * 0.7, 
                          button_color.g * 0.7, 
                          button_color.b * 0.7, 
                          button_color.a);
    SDL_RenderDrawRect(g_renderer, &button_rect);
    
    // Draw text centered on button (handle multi-line text)
    char text_copy[256];
    strncpy(text_copy, text, sizeof(text_copy) - 1);
    text_copy[sizeof(text_copy) - 1] = '\0';
    
    // Count number of lines
    int line_count = 1;
    char* temp_ptr = text_copy;
    while ((temp_ptr = strchr(temp_ptr, '\n')) != NULL) {
        line_count++;
        temp_ptr++;
    }
    
    // Calculate line height and starting y position
    int line_height = 30; // Increased spacing between lines
    int total_text_height = line_count * line_height;
    int start_y = y + h/2 - total_text_height/2 + line_height/2;
    
    // Render each line
    char* line = strtok(text_copy, "\n");
    int line_index = 0;
    while (line != NULL) {
        int line_y = start_y + line_index * line_height;
        draw_text(line, x + w/2, line_y, (SDL_Color){255, 255, 255, 255});
        line = strtok(NULL, "\n");
        line_index++;
    }
}

// Draw text centered at given position with normal font - exact implementation from app.c
void draw_text(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(g_font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(g_renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    SDL_Rect text_rect = {
        x - text_surface->w / 2, 
        y - text_surface->h / 2, 
        text_surface->w, 
        text_surface->h
    };
    
    SDL_RenderCopy(g_renderer, text_texture, NULL, &text_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

// Draw text centered at given position with large font - exact implementation from app.c
void draw_large_text(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(g_large_font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(g_renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    SDL_Rect text_rect = {
        x - text_surface->w / 2, 
        y - text_surface->h / 2, 
        text_surface->w, 
        text_surface->h
    };
    
    SDL_RenderCopy(g_renderer, text_texture, NULL, &text_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

// Draw text left-aligned at given position with normal font - exact implementation from app.c
void draw_text_left(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(g_font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(g_renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    SDL_Rect text_rect = {
        x, // Left-aligned, no centering
        y - text_surface->h / 2, // Still center vertically 
        text_surface->w, 
        text_surface->h
    };
    
    SDL_RenderCopy(g_renderer, text_texture, NULL, &text_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

// Draw text left-aligned with small font and width limit - exact implementation from app.c
void draw_small_text_left(const char* text, int x, int y, SDL_Color color, int max_width) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(g_small_font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(g_renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    // Calculate dimensions for rendering
    int width = text_surface->w;
    int height = text_surface->h;
    
    // If text exceeds max width, clip it
    if (width > max_width) {
        width = max_width;
    }
    
    SDL_Rect src_rect = {0, 0, width, height};
    SDL_Rect text_rect = {
        x, // Left-aligned
        y - height / 2, // Center vertically 
        width, 
        height
    };
    
    SDL_RenderCopy(g_renderer, text_texture, &src_rect, &text_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

// Render page indicators - exact implementation from app.c
void render_page_indicators(int current, int total, float transition) {
    // Calculate total width needed (including container padding)
    int total_dots_width = (total - 1) * PAGE_INDICATOR_SPACING + PAGE_INDICATOR_RADIUS * 2;
    int container_width = total_dots_width + PAGE_INDICATOR_CONTAINER_PADDING * 2;
    
    // Center the container
    int container_x = (actual_width - container_width) / 2;
    int container_y = PAGE_INDICATOR_Y - PAGE_INDICATOR_CONTAINER_HEIGHT / 2;
    
    // Draw container background (pill/capsule shape) with transparency
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 153); // Black with 60% opacity
    
    // Fill rounded rectangle (pill shape)
    int radius = PAGE_INDICATOR_CONTAINER_HEIGHT / 2;
    
    // Main rectangle
    SDL_Rect main_rect = {
        container_x + radius, 
        container_y, 
        container_width - 2 * radius, 
        PAGE_INDICATOR_CONTAINER_HEIGHT
    };
    SDL_RenderFillRect(g_renderer, &main_rect);
    
    // Left rounded end - draw filled semicircle
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= 0; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                SDL_RenderDrawPoint(g_renderer,
                    container_x + radius + dx,
                    container_y + radius + dy);
            }
        }
    }
    
    // Right rounded end - draw filled semicircle
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = 0; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                SDL_RenderDrawPoint(g_renderer,
                    container_x + container_width - radius + dx,
                    container_y + radius + dy);
            }
        }
    }
    
    // Draw page indicators
    int start_x = container_x + PAGE_INDICATOR_CONTAINER_PADDING;
    int target_page = -1;
    
    // Get target page from transition
    if (transition != 0) {
        if (transition > 0) {
            target_page = current - 1;
        } else {
            target_page = current + 1;
        }
    }
    
    for (int i = 0; i < total; i++) {
        int dot_x = start_x + i * PAGE_INDICATOR_SPACING + PAGE_INDICATOR_RADIUS;
        
        // Determine fill based on current page and transition
        bool filled = (i == current);
        int alpha = 255;
        
        // Adjust for transition
        if (target_page != -1) {
            if (i == current || i == target_page) {
                float t = fabs(transition);
                if (i == current) {
                    alpha = (int)(255 * (1.0f - t));
                } else {
                    alpha = (int)(255 * t);
                }
            }
        }
        
        // Choose color based on whether it's the active page
        SDL_Color dot_color;
        if (filled) {
            dot_color = (SDL_Color){255, 255, 255, 255}; // White for active
        } else {
            dot_color = (SDL_Color){150, 150, 150, 255}; // Gray for inactive
        }
        
        // Draw the indicator dot with full opacity but adjusted alpha for transitions
        SDL_SetRenderDrawColor(g_renderer, dot_color.r, dot_color.g, dot_color.b, alpha);
        
        // Draw filled circle
        for (int r = 0; r <= PAGE_INDICATOR_RADIUS; r++) {
            for (int angle = 0; angle < 360; angle += 5) {
                float rad = angle * M_PI / 180.0f;
                int x = dot_x + (int)(r * cos(rad));
                int y = PAGE_INDICATOR_Y + (int)(r * sin(rad));
                SDL_RenderDrawPoint(g_renderer, x, y);
            }
        }
    }
    
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
}