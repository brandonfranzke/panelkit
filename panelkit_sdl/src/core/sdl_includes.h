/**
 * @file sdl_includes.h
 * @brief Centralized SDL include management
 * 
 * This header provides a single point of control for SDL includes,
 * handling platform-specific paths and build configurations.
 */

#ifndef PANELKIT_SDL_INCLUDES_H
#define PANELKIT_SDL_INCLUDES_H

/*
 * SDL include strategy:
 * - For embedded builds: Always use <SDL2/SDL.h> (headers in prefix/include/SDL2/)
 * - For standard Linux: Use <SDL.h> (headers may be in /usr/include/SDL2/ with -I flag)  
 * - For macOS: Use <SDL.h> (CMakeLists adds SDL2 subdirectory to include path)
 * 
 * The CMakeLists.txt handles the include paths appropriately for each platform.
 */

#ifdef EMBEDDED_BUILD
    /* Embedded cross-compilation build */
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
#elif defined(__APPLE__)
    /* macOS with Homebrew - CMakeLists adds SDL2 subdir to include path */
    #include <SDL.h>
    #include <SDL_ttf.h>
#elif defined(__linux__)
    /* Standard Linux - may have headers in various locations */
    #include <SDL.h>
    #include <SDL_ttf.h>
#else
    /* Other platforms - try SDL2 subdirectory */
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
#endif

#endif /* PANELKIT_SDL_INCLUDES_H */