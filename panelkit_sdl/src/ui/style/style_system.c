#include "style_system.h"
#include "core/logger.h"
#include "core/error.h"

// Global font manager instance
FontManager* g_font_manager = NULL;

PkError style_system_init(void) {
    if (g_font_manager) {
        log_warn("Style system already initialized");
        return PK_OK;
    }
    
    PkError err = font_manager_create(&g_font_manager);
    if (err != PK_OK) {
        log_error("Failed to create font manager: %s", pk_error_string(err));
        return err;
    }
    
    // Load default font
    // TODO: Load from embedded font data or config
    log_info("Style system initialized");
    return PK_OK;
}

void style_system_cleanup(void) {
    if (g_font_manager) {
        font_manager_destroy(g_font_manager);
        g_font_manager = NULL;
        log_info("Style system cleaned up");
    }
}

FontManager* style_system_get_font_manager(void) {
    if (!g_font_manager) {
        log_warn("Style system not initialized");
    }
    return g_font_manager;
}