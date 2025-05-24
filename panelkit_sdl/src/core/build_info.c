/**
 * @file build_info.c
 * @brief Build information implementation
 */

#include "build_info.h"
#include "logger.h"
#include "sdl_includes.h"
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

/* Compile-time information */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/* Build timestamp */
const char* build_get_timestamp(void) {
    return __DATE__ " " __TIME__;
}

/* Compiler info */
const char* build_get_compiler(void) {
#ifdef __clang__
    return "Clang " __clang_version__;
#elif defined(__GNUC__)
    return "GCC " __VERSION__;
#else
    return "Unknown compiler";
#endif
}

/* Build type */
const char* build_get_type(void) {
#ifdef DEBUG
    return "Debug";
#else
    return "Release";
#endif
}

/* Platform info */
const char* build_get_platform(void) {
#ifdef __linux__
    #ifdef __aarch64__
        return "Linux ARM64";
    #elif defined(__arm__)
        return "Linux ARM32";
    #elif defined(__x86_64__)
        return "Linux x86_64";
    #else
        return "Linux (unknown arch)";
    #endif
#elif defined(__APPLE__)
    #ifdef __aarch64__
        return "macOS ARM64";
    #else
        return "macOS x86_64";
    #endif
#else
    return "Unknown platform";
#endif
}

/* Library information */
static const LibraryInfo libraries[] = {
    {
        .name = "SDL2",
        .version = TOSTRING(SDL_MAJOR_VERSION) "." TOSTRING(SDL_MINOR_VERSION) "." TOSTRING(SDL_PATCHLEVEL),
        .compile_flags = NULL,
#ifdef EMBEDDED_BUILD
        .is_static = true,
#else
        .is_static = false,
#endif
        .is_present = true
    },
    {
        .name = "SDL2_ttf",
        .version = TOSTRING(SDL_TTF_MAJOR_VERSION) "." TOSTRING(SDL_TTF_MINOR_VERSION) "." TOSTRING(SDL_TTF_PATCHLEVEL),
        .compile_flags = NULL,
#ifdef EMBEDDED_BUILD
        .is_static = true,
#else
        .is_static = false,
#endif
        .is_present = true
    },
    {
        .name = "libcurl",
        .version = LIBCURL_VERSION,
        .compile_flags = NULL,
#ifdef EMBEDDED_BUILD
        .is_static = true,
#else
        .is_static = false,
#endif
        .is_present = true
    },
    {
        .name = "libdrm",
        .version = "2.4.x",  // Version determined at runtime
        .compile_flags = NULL,
        .is_static = false,  // Always dynamic
#ifdef __linux__
        .is_present = true
#else
        .is_present = false
#endif
    }
};

const LibraryInfo* build_get_libraries(int* count) {
    if (count) {
        *count = sizeof(libraries) / sizeof(libraries[0]);
    }
    return libraries;
}

/* Log comprehensive build info */
void build_log_info(void) {
    log_info("=== Build Information ===");
    log_info("Build timestamp: %s", build_get_timestamp());
    log_info("Compiler: %s", build_get_compiler());
    log_info("Build type: %s", build_get_type());
    log_info("Platform: %s", build_get_platform());
    
    /* Compile-time defines */
    log_info("Compile-time features:");
#ifdef EMBEDDED_BUILD
    log_info("  EMBEDDED_BUILD: Yes");
#else
    log_info("  EMBEDDED_BUILD: No");
#endif
    
#ifdef DEBUG
    log_info("  DEBUG: Yes");
#else
    log_info("  DEBUG: No");
#endif
    
    /* Library information */
    log_info("Linked libraries:");
    int lib_count;
    const LibraryInfo* libs = build_get_libraries(&lib_count);
    for (int i = 0; i < lib_count; i++) {
        if (libs[i].is_present) {
            log_info("  %s v%s (%s)", 
                     libs[i].name, 
                     libs[i].version,
                     libs[i].is_static ? "static" : "dynamic");
        }
    }
    
    /* Memory info */
    log_info("Memory configuration:");
    log_info("  Pointer size: %zu bits", sizeof(void*) * 8);
    log_info("  Page size: %ld bytes", sysconf(_SC_PAGESIZE));
    
    log_info("========================");
}

/* SDL-specific build info */
void build_log_sdl_info(void) {
    log_info("=== SDL Build Configuration ===");
    
    /* SDL version */
    SDL_version compiled;
    SDL_version linked;
    
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    
    log_info("SDL2 compiled version: %d.%d.%d", 
             compiled.major, compiled.minor, compiled.patch);
    log_info("SDL2 linked version: %d.%d.%d",
             linked.major, linked.minor, linked.patch);
    
    /* SDL compile-time options */
    log_info("SDL2 build configuration:");
    
    /* Video drivers compiled in */
    log_info("  Available video drivers:");
    int num_drivers = SDL_GetNumVideoDrivers();
    for (int i = 0; i < num_drivers; i++) {
        const char* driver = SDL_GetVideoDriver(i);
        log_info("    %d: %s", i, driver);
    }
    
    /* Current video driver */
    const char* current_driver = SDL_GetCurrentVideoDriver();
    if (current_driver) {
        log_info("  Current video driver: %s", current_driver);
    } else {
        log_info("  Current video driver: None (not initialized)");
    }
    
    /* Platform */
    const char* platform = SDL_GetPlatform();
    log_info("  Platform: %s", platform);
    
    /* CPU info */
    log_info("  CPU cores: %d", SDL_GetCPUCount());
    log_info("  CPU cache line size: %d", SDL_GetCPUCacheLineSize());
    log_info("  System RAM: %d MB", SDL_GetSystemRAM());
    
    /* Feature detection */
    log_info("  CPU features:");
    if (SDL_HasRDTSC()) log_info("    RDTSC: Yes");
    if (SDL_HasAltiVec()) log_info("    AltiVec: Yes");
    if (SDL_HasMMX()) log_info("    MMX: Yes");
    if (SDL_HasSSE()) log_info("    SSE: Yes");
    if (SDL_HasSSE2()) log_info("    SSE2: Yes");
    if (SDL_HasSSE3()) log_info("    SSE3: Yes");
    if (SDL_HasSSE41()) log_info("    SSE4.1: Yes");
    if (SDL_HasSSE42()) log_info("    SSE4.2: Yes");
    if (SDL_HasAVX()) log_info("    AVX: Yes");
    if (SDL_HasAVX2()) log_info("    AVX2: Yes");
    if (SDL_HasNEON()) log_info("    NEON: Yes");
    
    /* Hints */
    log_info("  SDL Hints:");
    const char* hint_driver = SDL_GetHint(SDL_HINT_VIDEODRIVER);
    if (hint_driver) {
        log_info("    SDL_VIDEODRIVER: %s", hint_driver);
    }
    
    const char* hint_render_driver = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
    if (hint_render_driver) {
        log_info("    SDL_RENDER_DRIVER: %s", hint_render_driver);
    }
    
    /* Input subsystem */
    if (SDL_WasInit(SDL_INIT_EVENTS)) {
        log_info("  Event subsystem: Initialized");
        
        /* Touch devices */
        int num_touch = SDL_GetNumTouchDevices();
        log_info("  Touch devices: %d", num_touch);
        for (int i = 0; i < num_touch; i++) {
            SDL_TouchID id = SDL_GetTouchDevice(i);
            log_info("    Device %d: ID=%lld", i, (long long)id);
        }
    } else {
        log_info("  Event subsystem: Not initialized");
    }
    
    log_info("===============================");
}

/* Check for specific features */
bool build_has_feature(const char* feature) {
    if (!feature) return false;
    
    if (strcmp(feature, "embedded") == 0) {
#ifdef EMBEDDED_BUILD
        return true;
#else
        return false;
#endif
    }
    
    if (strcmp(feature, "debug") == 0) {
#ifdef DEBUG
        return true;
#else
        return false;
#endif
    }
    
    if (strcmp(feature, "drm") == 0) {
#ifdef __linux__
        return true;
#else
        return false;
#endif
    }
    
    if (strcmp(feature, "static_sdl") == 0) {
#ifdef EMBEDDED_BUILD
        return true;
#else
        return false;
#endif
    }
    
    return false;
}