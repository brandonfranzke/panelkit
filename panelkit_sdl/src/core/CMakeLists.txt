# Core components library
add_library(panelkit_core STATIC
    logger.c
    build_info.c
    error.c
)

# Find zlog
if(EMBEDDED_BUILD)
    # For embedded builds, use static zlog from cross-compilation environment
    set(ZLOG_INCLUDE_DIRS "/usr/local/aarch64/include")
    set(ZLOG_LIBRARIES "/usr/local/aarch64/lib/libzlog.a")
else()
    # Development build - find zlog
    find_path(ZLOG_INCLUDE_DIR zlog.h
        HINTS /usr/local/include /usr/include
    )
    find_library(ZLOG_LIBRARY zlog
        HINTS /usr/local/lib /usr/lib
    )
    
    if(ZLOG_INCLUDE_DIR AND ZLOG_LIBRARY)
        set(ZLOG_INCLUDE_DIRS ${ZLOG_INCLUDE_DIR})
        set(ZLOG_LIBRARIES ${ZLOG_LIBRARY})
        message(STATUS "Found zlog: ${ZLOG_LIBRARY}")
    else()
        message(FATAL_ERROR "zlog not found. Please install zlog library.")
    endif()
endif()

target_include_directories(panelkit_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLOG_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# SDL include directories (handle macOS homebrew case)
if(APPLE AND NOT EMBEDDED_BUILD)
    target_include_directories(panelkit_core PUBLIC
        ${SDL2_INCLUDE_DIRS}/SDL2
        ${SDL2_TTF_INCLUDE_DIRS}/SDL2
    )
else()
    target_include_directories(panelkit_core PUBLIC
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )
endif()

target_link_libraries(panelkit_core PUBLIC
    ${ZLOG_LIBRARIES}
)