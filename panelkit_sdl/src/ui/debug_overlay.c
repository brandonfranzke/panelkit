/**
 * @file debug_overlay.c
 * @brief Debug overlay implementation with minimal rendering
 * 
 * This is a minimal implementation that provides the infrastructure
 * for debug overlays without full UI implementation.
 */

#include "debug_overlay.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Maximum custom metrics and info providers */
#define MAX_CUSTOM_METRICS 16
#define MAX_CUSTOM_INFO 8

/* Custom metric entry */
typedef struct {
    char name[64];
    metric_getter_func getter;
    void* user_data;
} CustomMetric;

/* Custom info entry */
typedef struct {
    char name[64];
    info_formatter_func formatter;
    void* user_data;
} CustomInfo;

/* Debug overlay implementation */
struct DebugOverlay {
    DebugOverlayConfig config;
    bool visible;
    
    /* Timing */
    uint32_t last_update_ms;
    double accumulated_time;
    int frame_count;
    
    /* Custom providers */
    CustomMetric metrics[MAX_CUSTOM_METRICS];
    size_t metric_count;
    CustomInfo infos[MAX_CUSTOM_INFO];
    size_t info_count;
    
    /* Cached data */
    PerfMetrics perf_cache;
    MemoryStats memory_cache;
    PkError last_error;
    char last_error_context[256];
};

/* Global metrics storage (simplified) */
static struct {
    PerfMetrics perf;
    MemoryStats memory;
    uint32_t frame_start_ms;
    uint32_t render_start_ms;
} g_debug_metrics = {0};

DebugOverlayConfig debug_overlay_default_config(DebugOverlayType type) {
    DebugOverlayConfig config = {
        .type = type,
        .categories = (type == DEBUG_OVERLAY_STRIP) ? 
                     (DEBUG_CAT_ERRORS | DEBUG_CAT_PERF) : DEBUG_CAT_ALL,
        .update_interval_ms = DEBUG_OVERLAY_DEFAULT_UPDATE_MS,
        .show_on_startup = false,
        .background_color = {0, 0, 0, 200},
        .text_color = {255, 255, 255, 255},
        .error_color = {255, 64, 64, 255},
        .warning_color = {255, 200, 64, 255},
        .font_size = DEBUG_OVERLAY_DEFAULT_FONT_SIZE,
        .opacity = DEBUG_OVERLAY_DEFAULT_OPACITY
    };
    
    /* Strip has different defaults */
    if (type == DEBUG_OVERLAY_STRIP) {
        config.background_color.a = 150;
        config.font_size = 10;
    }
    
    return config;
}

DebugOverlay* debug_overlay_create(const DebugOverlayConfig* config) {
    DebugOverlay* overlay = calloc(1, sizeof(DebugOverlay));
    if (!overlay) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "debug_overlay_create: Failed to allocate %zu bytes", sizeof(DebugOverlay));
        return NULL;
    }
    
    /* Use provided config or defaults */
    if (config) {
        overlay->config = *config;
    } else {
        overlay->config = debug_overlay_default_config(DEBUG_OVERLAY_MAIN);
    }
    
    overlay->visible = overlay->config.show_on_startup;
    overlay->last_update_ms = SDL_GetTicks();
    
    log_info("Created debug overlay (type=%d, categories=0x%x)", 
             overlay->config.type, overlay->config.categories);
    
    return overlay;
}

void debug_overlay_destroy(DebugOverlay* overlay) {
    if (!overlay) {
        return;
    }
    
    log_info("Destroyed debug overlay");
    free(overlay);
}

void debug_overlay_update(DebugOverlay* overlay, double delta_time) {
    if (!overlay || !overlay->visible) {
        return;
    }
    
    overlay->accumulated_time += delta_time;
    overlay->frame_count++;
    
    /* Check if update needed */
    uint32_t now_ms = SDL_GetTicks();
    if (now_ms - overlay->last_update_ms >= (uint32_t)overlay->config.update_interval_ms) {
        /* Update cached metrics */
        overlay->perf_cache = g_debug_metrics.perf;
        overlay->memory_cache = g_debug_metrics.memory;
        
        /* Calculate FPS from accumulated data */
        if (overlay->accumulated_time > 0) {
            overlay->perf_cache.fps = overlay->frame_count / overlay->accumulated_time;
        }
        
        /* Get last error */
        overlay->last_error = pk_get_last_error();
        const char* context = pk_get_last_error_context();
        if (context) {
            strncpy(overlay->last_error_context, context, sizeof(overlay->last_error_context) - 1);
        }
        
        /* Reset accumulators */
        overlay->accumulated_time = 0;
        overlay->frame_count = 0;
        overlay->last_update_ms = now_ms;
    }
}

PkError debug_overlay_render(DebugOverlay* overlay, SDL_Renderer* renderer) {
    if (!overlay || !overlay->visible || !renderer) {
        return PK_OK;
    }
    
    /* NOTE: This is a minimal implementation that just logs the info
     * A full implementation would render text using TTF or a bitmap font */
    
    /* For now, we'll prepare the debug text that would be rendered */
    char debug_text[1024];
    int offset = 0;
    
    if (overlay->config.type == DEBUG_OVERLAY_STRIP) {
        /* Minimal strip format */
        offset = snprintf(debug_text, sizeof(debug_text),
            DEBUG_STRIP_CONTENT,
            overlay->last_error,
            overlay->memory_cache.heap_used / (1024 * 1024),
            overlay->perf_cache.fps);
    } else {
        /* Full overlay format */
        offset = snprintf(debug_text, sizeof(debug_text),
            "=== Debug Overlay ===\n");
        
        if (overlay->config.categories & DEBUG_CAT_ERRORS) {
            offset += snprintf(debug_text + offset, sizeof(debug_text) - offset,
                "Error: %d - %s\n", 
                overlay->last_error,
                overlay->last_error_context);
        }
        
        if (overlay->config.categories & DEBUG_CAT_PERF) {
            offset += snprintf(debug_text + offset, sizeof(debug_text) - offset,
                "FPS: %.1f (%.1fms)\n",
                overlay->perf_cache.fps,
                overlay->perf_cache.frame_time_ms);
        }
        
        if (overlay->config.categories & DEBUG_CAT_MEMORY) {
            offset += snprintf(debug_text + offset, sizeof(debug_text) - offset,
                "Memory: %zuMB / %zuMB peak\n",
                overlay->memory_cache.heap_used / (1024 * 1024),
                overlay->memory_cache.heap_peak / (1024 * 1024));
        }
        
        /* Add custom metrics */
        for (size_t i = 0; i < overlay->metric_count; i++) {
            float value = overlay->metrics[i].getter(overlay->metrics[i].user_data);
            offset += snprintf(debug_text + offset, sizeof(debug_text) - offset,
                "%s: %.2f\n", overlay->metrics[i].name, value);
        }
    }
    
    /* In a real implementation, this would render the text */
    /* For now, we can at least draw a background rectangle */
    SDL_Rect rect;
    if (overlay->config.type == DEBUG_OVERLAY_STRIP) {
        /* Bottom strip */
        int window_w, window_h;
        SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
        rect.x = 0;
        rect.y = window_h - 30;
        rect.w = window_w;
        rect.h = 30;
    } else {
        /* Top-right corner panel */
        rect.x = 10;
        rect.y = 10;
        rect.w = 300;
        rect.h = 200;
    }
    
    /* Draw semi-transparent background */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 
        overlay->config.background_color.r,
        overlay->config.background_color.g,
        overlay->config.background_color.b,
        (Uint8)(overlay->config.background_color.a * overlay->config.opacity));
    SDL_RenderFillRect(renderer, &rect);
    
    /* Log debug info for development */
    static uint32_t last_log_ms = 0;
    uint32_t now_ms = SDL_GetTicks();
    if (now_ms - last_log_ms > 5000) {  /* Log every 5 seconds */
        log_debug("Debug overlay: %s", debug_text);
        last_log_ms = now_ms;
    }
    
    return PK_OK;
}

void debug_overlay_toggle(DebugOverlay* overlay) {
    if (overlay) {
        overlay->visible = !overlay->visible;
        log_info("Debug overlay %s", overlay->visible ? "shown" : "hidden");
    }
}

void debug_overlay_set_visible(DebugOverlay* overlay, bool visible) {
    if (overlay) {
        overlay->visible = visible;
    }
}

bool debug_overlay_is_visible(const DebugOverlay* overlay) {
    return overlay ? overlay->visible : false;
}

void debug_overlay_set_categories(DebugOverlay* overlay, unsigned int categories) {
    if (overlay) {
        overlay->config.categories = categories;
    }
}

void debug_overlay_register_metric(DebugOverlay* overlay, 
                                 const char* name,
                                 metric_getter_func getter,
                                 void* user_data) {
    if (!overlay || !name || !getter || overlay->metric_count >= MAX_CUSTOM_METRICS) {
        return;
    }
    
    CustomMetric* metric = &overlay->metrics[overlay->metric_count++];
    strncpy(metric->name, name, sizeof(metric->name) - 1);
    metric->getter = getter;
    metric->user_data = user_data;
}

void debug_overlay_register_info(DebugOverlay* overlay,
                               const char* name,
                               info_formatter_func formatter,
                               void* user_data) {
    if (!overlay || !name || !formatter || overlay->info_count >= MAX_CUSTOM_INFO) {
        return;
    }
    
    CustomInfo* info = &overlay->infos[overlay->info_count++];
    strncpy(info->name, name, sizeof(info->name) - 1);
    info->formatter = formatter;
    info->user_data = user_data;
}

/* Global metrics collection */

void debug_metrics_record_frame(uint32_t frame_start_ms, uint32_t frame_end_ms) {
    g_debug_metrics.frame_start_ms = frame_start_ms;
    g_debug_metrics.perf.frame_time_ms = (float)(frame_end_ms - frame_start_ms);
}

void debug_metrics_record_render(uint32_t render_start_ms, uint32_t render_end_ms, 
                               size_t draw_calls) {
    g_debug_metrics.render_start_ms = render_start_ms;
    g_debug_metrics.perf.render_time_ms = (float)(render_end_ms - render_start_ms);
    g_debug_metrics.perf.draw_calls = draw_calls;
}

void debug_metrics_update_memory(const MemoryStats* stats) {
    if (stats) {
        g_debug_metrics.memory = *stats;
    }
}

void debug_metrics_get_perf(PerfMetrics* metrics) {
    if (metrics) {
        *metrics = g_debug_metrics.perf;
    }
}

void debug_metrics_get_memory(MemoryStats* stats) {
    if (stats) {
        *stats = g_debug_metrics.memory;
    }
}