/**
 * @file config_utils.h
 * @brief Configuration utility functions
 * 
 * Helper functions for configuration validation and manipulation.
 */

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <stdbool.h>

// Parse a boolean value from string
// Accepts: true/false, yes/no, on/off, 1/0 (case insensitive)
bool parse_bool(const char* value, bool* result);

#endif // CONFIG_UTILS_H