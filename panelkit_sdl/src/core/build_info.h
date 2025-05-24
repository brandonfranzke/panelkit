/**
 * @file build_info.h
 * @brief Build information and library introspection
 * 
 * Provides compile-time and runtime information about the build,
 * including statically linked libraries and their configurations.
 */

#ifndef PANELKIT_BUILD_INFO_H
#define PANELKIT_BUILD_INFO_H

#include <stdbool.h>

/* Build configuration info */
typedef struct {
    const char* name;
    const char* version;
    const char* compile_flags;
    bool is_static;
    bool is_present;
} LibraryInfo;

/* Get build timestamp */
const char* build_get_timestamp(void);

/* Get compiler info */
const char* build_get_compiler(void);

/* Get build type (Debug/Release) */
const char* build_get_type(void);

/* Get build platform */
const char* build_get_platform(void);

/* Get linked libraries info */
const LibraryInfo* build_get_libraries(int* count);

/* Print comprehensive build info to logs */
void build_log_info(void);

/* Get SDL specific build info */
void build_log_sdl_info(void);

/* Check if a specific feature is enabled */
bool build_has_feature(const char* feature);

#endif /* PANELKIT_BUILD_INFO_H */