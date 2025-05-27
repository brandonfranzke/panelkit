/**
 * @file debug_overlay.h
 * @brief Debug overlay system for displaying system metrics and debug info
 * 
 * This module provides a flexible debug overlay system with two main components:
 * 1. Main overlay - comprehensive debug information panel
 * 2. Status strip - minimal always-visible debug info
 * 
 * The system is designed to be compile-time configurable for different
 * debugging scenarios (input, performance, errors, etc.)
 */

#ifndef PANELKIT_DEBUG_OVERLAY_H
#define PANELKIT_DEBUG_OVERLAY_H

#include "../core/sdl_includes.h"
#include "../core/error.h"
#include <stdbool.h>
#include <stddef.h>

/* Debug overlay types */
typedef enum {
    DEBUG_OVERLAY_MAIN,      /* Full overlay with comprehensive info */
    DEBUG_OVERLAY_STRIP,     /* Bottom strip with minimal info */
    DEBUG_OVERLAY_CUSTOM     /* User-defined overlay */
} DebugOverlayType;

/* Debug info categories */
typedef enum {
    DEBUG_CAT_ERRORS    = (1 << 0),  /* Error state and history */
    DEBUG_CAT_MEMORY    = (1 << 1),  /* Memory usage stats */
    DEBUG_CAT_PERF      = (1 << 2),  /* Performance metrics */
    DEBUG_CAT_INPUT     = (1 << 3),  /* Input events and state */
    DEBUG_CAT_NETWORK   = (1 << 4),  /* Network/API status */
    DEBUG_CAT_WIDGETS   = (1 << 5),  /* Widget tree info */
    DEBUG_CAT_EVENTS    = (1 << 6),  /* Event system stats */
    DEBUG_CAT_STATE     = (1 << 7),  /* State store contents */
    DEBUG_CAT_ALL       = 0xFFFF      /* All categories */
} DebugCategory;

/* Performance metrics */
typedef struct {
    float fps;                  /* Current frames per second */
    float frame_time_ms;        /* Last frame time in milliseconds */
    float update_time_ms;       /* Widget update time */
    float render_time_ms;       /* Rendering time */
    size_t draw_calls;          /* Number of draw calls per frame */
} PerfMetrics;

/* Memory statistics */
typedef struct {
    size_t heap_used;           /* Current heap usage */
    size_t heap_peak;           /* Peak heap usage */
    size_t widget_count;        /* Total widgets allocated */
    size_t texture_memory;      /* GPU texture memory used */
    size_t event_queue_size;    /* Events in queue */
} MemoryStats;

/* Debug overlay configuration */
typedef struct {
    DebugOverlayType type;      /* Type of overlay */
    unsigned int categories;    /* Bitmask of categories to show */
    int update_interval_ms;     /* How often to update stats */
    bool show_on_startup;       /* Show immediately on init */
    SDL_Color background_color; /* Overlay background */
    SDL_Color text_color;       /* Default text color */
    SDL_Color error_color;      /* Error text color */
    SDL_Color warning_color;    /* Warning text color */
    int font_size;             /* Font size for overlay text */
    float opacity;             /* Overlay opacity (0.0-1.0) */
} DebugOverlayConfig;

/* Debug overlay handle */
typedef struct DebugOverlay DebugOverlay;

/**
 * Create a debug overlay.
 * 
 * @param config Configuration for the overlay
 * @return New overlay instance or NULL on error
 */
DebugOverlay* debug_overlay_create(const DebugOverlayConfig* config);

/**
 * Destroy a debug overlay.
 * 
 * @param overlay Overlay to destroy
 */
void debug_overlay_destroy(DebugOverlay* overlay);

/**
 * Update debug overlay metrics.
 * 
 * @param overlay The overlay instance
 * @param delta_time Time since last update in seconds
 * 
 * @note This should be called each frame
 */
void debug_overlay_update(DebugOverlay* overlay, double delta_time);

/**
 * Render the debug overlay.
 * 
 * @param overlay The overlay instance
 * @param renderer SDL renderer
 * @return PK_OK on success, error code on failure
 * 
 * @note This should be called after all other rendering
 */
PkError debug_overlay_render(DebugOverlay* overlay, SDL_Renderer* renderer);

/**
 * Toggle overlay visibility.
 * 
 * @param overlay The overlay instance
 */
void debug_overlay_toggle(DebugOverlay* overlay);

/**
 * Show/hide the overlay.
 * 
 * @param overlay The overlay instance
 * @param visible true to show, false to hide
 */
void debug_overlay_set_visible(DebugOverlay* overlay, bool visible);

/**
 * Check if overlay is visible.
 * 
 * @param overlay The overlay instance
 * @return true if visible
 */
bool debug_overlay_is_visible(const DebugOverlay* overlay);

/**
 * Set which categories to display.
 * 
 * @param overlay The overlay instance
 * @param categories Bitmask of categories
 */
void debug_overlay_set_categories(DebugOverlay* overlay, unsigned int categories);

/**
 * Register custom metrics provider.
 * 
 * @param overlay The overlay instance
 * @param name Name of the metric
 * @param getter Function to get metric value
 * @param user_data User data for getter
 */
typedef float (*metric_getter_func)(void* user_data);
void debug_overlay_register_metric(DebugOverlay* overlay, 
                                 const char* name,
                                 metric_getter_func getter,
                                 void* user_data);

/**
 * Register custom info provider.
 * 
 * @param overlay The overlay instance
 * @param name Name of the info section
 * @param formatter Function to format info
 * @param user_data User data for formatter
 */
typedef void (*info_formatter_func)(char* buffer, size_t size, void* user_data);
void debug_overlay_register_info(DebugOverlay* overlay,
                               const char* name,
                               info_formatter_func formatter,
                               void* user_data);

/* Global metrics collection hooks */

/**
 * Record frame timing.
 * 
 * @param frame_start_ms Frame start time in milliseconds
 * @param frame_end_ms Frame end time in milliseconds
 */
void debug_metrics_record_frame(uint32_t frame_start_ms, uint32_t frame_end_ms);

/**
 * Record render timing.
 * 
 * @param render_start_ms Render start time in milliseconds
 * @param render_end_ms Render end time in milliseconds
 * @param draw_calls Number of draw calls
 */
void debug_metrics_record_render(uint32_t render_start_ms, uint32_t render_end_ms, 
                               size_t draw_calls);

/**
 * Update memory statistics.
 * 
 * @param stats Current memory statistics
 */
void debug_metrics_update_memory(const MemoryStats* stats);

/**
 * Get current performance metrics.
 * 
 * @param metrics Output structure to fill
 */
void debug_metrics_get_perf(PerfMetrics* metrics);

/**
 * Get current memory statistics.
 * 
 * @param stats Output structure to fill
 */
void debug_metrics_get_memory(MemoryStats* stats);

/* Compile-time configuration for strip content */
#ifdef PANELKIT_DEBUG_STRIP_INPUT
    #define DEBUG_STRIP_CONTENT "Input: Touch(%d,%d) Click:%s Gesture:%s"
#elif defined(PANELKIT_DEBUG_STRIP_SCROLL)
    #define DEBUG_STRIP_CONTENT "Scroll: Pos:%d Max:%d Vel:%.1f"
#elif defined(PANELKIT_DEBUG_STRIP_NETWORK)
    #define DEBUG_STRIP_CONTENT "Net: API:%s Queue:%d Retry:%d"
#elif defined(PANELKIT_DEBUG_STRIP_PERF)
    #define DEBUG_STRIP_CONTENT "FPS:%.1f Frame:%.1fms Draw:%zu"
#else
    #define DEBUG_STRIP_CONTENT "Err:%d Mem:%zuMB FPS:%.1f"
#endif

/* Default configuration */
#define DEBUG_OVERLAY_DEFAULT_UPDATE_MS 100
#define DEBUG_OVERLAY_DEFAULT_FONT_SIZE 12
#define DEBUG_OVERLAY_DEFAULT_OPACITY 0.85f

/**
 * Get default debug overlay configuration.
 * 
 * @param type Type of overlay
 * @return Configuration with sensible defaults
 */
DebugOverlayConfig debug_overlay_default_config(DebugOverlayType type);

#endif /* PANELKIT_DEBUG_OVERLAY_H */