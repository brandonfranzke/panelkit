# Display backend library
add_library(panelkit_display STATIC
    display_backend.c
    backend_sdl.c
    backend_sdl_drm.c
)

# Include directories
target_include_directories(panelkit_display PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# SDL include directories (handle macOS homebrew case)
if(APPLE AND NOT EMBEDDED_BUILD)
    target_include_directories(panelkit_display PUBLIC
        ${SDL2_INCLUDE_DIRS}/SDL2
        ${SDL2_TTF_INCLUDE_DIRS}/SDL2
    )
else()
    target_include_directories(panelkit_display PUBLIC
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )
endif()

# Platform-specific dependencies
if(EMBEDDED_BUILD)
    # For embedded builds, we need DRM headers
    target_include_directories(panelkit_display PRIVATE
        /usr/include/libdrm
        /usr/include/aarch64-linux-gnu
    )
    # Link against libdrm for DRM backend
    target_link_libraries(panelkit_display PUBLIC
        drm
    )
else()
    # For host builds, only link DRM on Linux
    if(UNIX AND NOT APPLE)
        find_library(DRM_LIBRARY drm)
        if(DRM_LIBRARY)
            target_link_libraries(panelkit_display PUBLIC ${DRM_LIBRARY})
        endif()
    endif()
endif()

# Link against core for logging
target_link_libraries(panelkit_display PUBLIC panelkit_core)