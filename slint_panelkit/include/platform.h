#ifndef PLATFORM_H
#define PLATFORM_H

#include "lvgl/lvgl.h"

/**
 * Initialize the platform-specific display and input drivers
 * @return 0 on success, negative value on failure
 */
int platform_init(void);

/**
 * Clean up platform resources
 */
void platform_deinit(void);

/**
 * Handle platform-specific tasks
 * @return 0 to continue running, non-zero to exit the application
 */
int platform_handle_tasks(void);

#endif /* PLATFORM_H */