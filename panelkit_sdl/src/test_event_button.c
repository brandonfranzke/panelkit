#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include "event_button_poc.h"
#include "state/state_store.h"
#include "events/event_system.h"

// Simple logging for test program
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        LOG_ERROR("SDL_ttf initialization failed: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Event Button POC Test",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        LOG_ERROR("Window creation failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
                                               SDL_RENDERER_ACCELERATED | 
                                               SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        LOG_ERROR("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    TTF_Font* font = TTF_OpenFont("fonts/font-sans-regular.ttf", 16);
    if (!font) {
        LOG_ERROR("Failed to load font: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    StateStore* state_store = state_store_create();
    if (!state_store) {
        LOG_ERROR("Failed to create state store");
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    EventSystem* event_system = event_system_create();
    if (!event_system) {
        LOG_ERROR("Failed to create event system");
        state_store_destroy(state_store);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    EventButtonPOC* button = event_button_poc_create(300, 250, 200, 100, 
                                                    "Weather", 
                                                    "weather.temperature");
    if (!button) {
        LOG_ERROR("Failed to create event button");
        event_system_destroy(event_system);
        state_store_destroy(state_store);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Subscribe button to event system
    if (!event_button_poc_subscribe(button, event_system, state_store)) {
        LOG_ERROR("Failed to subscribe button to events");
        event_button_poc_destroy(button);
        event_system_destroy(event_system);
        state_store_destroy(state_store);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    LOG_INFO("Event Button POC Test Started");
    LOG_INFO("Click the button to simulate weather updates");
    LOG_INFO("The button will display temperature when weather events are received");
    
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Check if click is on button
                    if (event.button.x >= button->rect.x && 
                        event.button.x < button->rect.x + button->rect.w &&
                        event.button.y >= button->rect.y && 
                        event.button.y < button->rect.y + button->rect.h) {
                        // Simulate weather event on button click
                        event_button_poc_simulate_weather_event(event_system, 72.5f, "New York");
                        LOG_INFO("Simulated weather event: 72.5°F in New York");
                    }
                }
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);
        
        event_button_poc_render(button, renderer);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_Rect instructions_rect = {50, 50, 700, 30};
        SDL_RenderDrawRect(renderer, &instructions_rect);
        
        SDL_Color text_color = {0, 0, 0, 255};
        const char* instructions = "Click the button to simulate weather data events";
        SDL_Surface* text_surface = TTF_RenderText_Solid(font, instructions, text_color);
        if (text_surface) {
            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            if (text_texture) {
                SDL_Rect text_rect = {60, 55, text_surface->w, text_surface->h};
                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    LOG_INFO("Shutting down Event Button POC Test");
    
    event_button_poc_destroy(button);
    event_system_destroy(event_system);
    state_store_destroy(state_store);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}