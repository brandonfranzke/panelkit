/**
 * @file test_logger.c
 * @brief Test program for the logging system
 */

#include "../src/core/logger.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    const char* config_path = NULL;
    
    /* Check for custom config path */
    if (argc > 1) {
        config_path = argv[1];
    }
    
    /* Initialize logging */
    printf("Initializing logger with config: %s\n", 
           config_path ? config_path : "default");
    
    if (!logger_init(config_path, "test_logger")) {
        fprintf(stderr, "Note: Using fallback logging to stderr\n");
    }
    
    /* Test all log levels */
    log_info("=== Logger Test Starting ===");
    
    log_debug("This is a debug message");
    log_info("This is an info message");
    log_notice("This is a notice message");
    log_warn("This is a warning message");
    log_error("This is an error message");
    
    /* Test system info logging */
    log_system_info();
    log_build_info();
    log_display_info(1920, 1080, "TEST");
    
    /* Test structured logging */
    log_event("TEST_EVENT", "param1=%d param2=%s", 42, "test");
    log_state_change("TestComponent", "IDLE", "ACTIVE");
    
    /* Test error helpers (without actual errors) */
    log_info("Testing error macros (simulated):");
    /* These would normally be used with actual error conditions:
     * LOG_SDL_ERROR("SDL initialization failed");
     * LOG_ERRNO("File open failed");
     * LOG_DRM_ERROR("DRM setup failed", -1);
     */
    
    /* Test memory usage */
    log_memory_usage();
    
    /* Test frame time logging */
    log_info("Simulating frame times...");
    for (int i = 0; i < 10; i++) {
        log_frame_time(16.67f + (rand() % 5));
        usleep(100000); /* 100ms */
    }
    
    /* Test assertion (commented out as it would abort) */
    /* LOG_ASSERT(1 == 2, "This would abort the program"); */
    
    /* Final state change */
    log_state_change("TestComponent", "ACTIVE", "SHUTDOWN");
    
    log_info("=== Logger Test Complete ===");
    logger_shutdown();
    
    printf("\nTest complete. Check log files at:\n");
    printf("  /var/log/panelkit/panelkit.log\n");
    printf("  /var/log/panelkit/error.log\n");
    
    return 0;
}