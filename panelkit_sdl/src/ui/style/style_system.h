#ifndef PK_STYLE_SYSTEM_H
#define PK_STYLE_SYSTEM_H

#include "font_manager.h"

// Global font manager instance
// Must be initialized by the application before using styles
extern FontManager* g_font_manager;

// Initialize the style system
// Creates and sets up the global font manager
// Returns PK_OK on success
PkError style_system_init(void);

// Cleanup the style system
// Destroys the global font manager
void style_system_cleanup(void);

// Get the global font manager
// Returns NULL if not initialized
FontManager* style_system_get_font_manager(void);

#endif // PK_STYLE_SYSTEM_H