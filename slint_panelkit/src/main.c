#include "platform.h"
#include "ui_elements.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // For usleep()

int main(int argc, char *argv[])
{
    printf("Starting PanelKit Application\n");
    printf("Screen dimensions: 640x480 (portrait)\n");
    
    // Initialize platform (display and input)
    if (platform_init() != 0) {
        fprintf(stderr, "Failed to initialize platform\n");
        return EXIT_FAILURE;
    }
    
    // Get active screen
    lv_obj_t *screen = lv_scr_act();
    
    // Initialize UI elements
    ui_init(screen);
    
    printf("UI initialized\n");
    
    // Main event loop
    while (1) {
        // Handle platform-specific tasks and LVGL events
        if (platform_handle_tasks() != 0) {
            break;  // Exit requested
        }
        
        // Sleep to reduce CPU usage (5ms)
        usleep(5000);
    }
    
    // Clean up
    platform_deinit();
    
    printf("Application terminated normally\n");
    return EXIT_SUCCESS;
}