cmake_minimum_required(VERSION 3.10)
project(panelkit C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Debug flags
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -Wall -Wextra -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -Wall -Wextra")

# Embedded build configuration
if(EMBEDDED_BUILD)
    # Minimal embedded build using custom SDL2 built from source
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")
    message(STATUS "Embedded build using minimal SDL2 from source")
    
    # Add compile definition for embedded build
    add_compile_definitions(EMBEDDED_BUILD)
    
    # Use custom-built SDL2 from source
    set(CMAKE_PREFIX_PATH "/usr/local/aarch64")
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    
    # Static libraries built from source
    set(SDL2_STATIC_LIBRARIES "/usr/local/aarch64/lib/libSDL2.a")
    set(SDL2_TTF_STATIC_LIBRARIES "/usr/local/aarch64/lib/libSDL2_ttf.a")
    
    # Set include directories
    set(SDL2_INCLUDE_DIRS "/usr/local/aarch64/include")
    set(SDL2_TTF_INCLUDE_DIRS "/usr/local/aarch64/include")
    
    # Static curl from our custom build
    set(CURL_STATIC_LIBRARIES "/usr/local/aarch64/lib/libcurl.a")
    set(CURL_INCLUDE_DIRS "/usr/local/aarch64/include")
    
    message(STATUS "Using minimal SDL2 from: ${CMAKE_PREFIX_PATH}")
else()
    # Development build (macOS with Homebrew)
    find_package(PkgConfig REQUIRED)
    
    if(APPLE)
        # For macOS with Homebrew
        execute_process(
            COMMAND brew --prefix sdl2
            OUTPUT_VARIABLE SDL2_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        if(SDL2_PREFIX)
            set(SDL2_INCLUDE_DIRS "${SDL2_PREFIX}/include")
            set(SDL2_LIBRARIES "-L${SDL2_PREFIX}/lib -lSDL2")
            message(STATUS "Found SDL2 using Homebrew at: ${SDL2_PREFIX}")
        else()
            pkg_check_modules(SDL2 REQUIRED sdl2)
        endif()
        
        execute_process(
            COMMAND brew --prefix sdl2_ttf
            OUTPUT_VARIABLE SDL2_TTF_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        if(SDL2_TTF_PREFIX)
            set(SDL2_TTF_INCLUDE_DIRS "${SDL2_TTF_PREFIX}/include")
            set(SDL2_TTF_LIBRARIES "-L${SDL2_TTF_PREFIX}/lib -lSDL2_ttf")
            message(STATUS "Found SDL2_ttf using Homebrew at: ${SDL2_TTF_PREFIX}")
        else()
            pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
        endif()
        
        # Standard curl for development
        find_package(CURL REQUIRED)
    else()
        # Linux development build
        pkg_check_modules(SDL2 REQUIRED sdl2)
        pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
        find_package(CURL REQUIRED)
    endif()
endif()

# Source files
set(SOURCES
    src/app.c
    # src/api_functions.c  # Replaced by modular API system
    src/ui/gestures.c
    src/ui/pages.c
    src/ui/rendering.c
    src/api/api_client.c
    src/api/api_manager.c
    src/json/json_parser.c
    src/json/jsmn.c
)

# Core library sources
add_subdirectory(src/core)

# Display backend sources
add_subdirectory(src/display)

# Input handler sources
add_subdirectory(src/input)

# Include directories
if(APPLE AND NOT EMBEDDED_BUILD)
    include_directories(${SDL2_INCLUDE_DIRS}/SDL2 ${SDL2_TTF_INCLUDE_DIRS}/SDL2)
elseif(EMBEDDED_BUILD)
    # For embedded build, SDL headers are in SDL2 subdirectory
    include_directories(${SDL2_INCLUDE_DIRS}/SDL2 ${SDL2_TTF_INCLUDE_DIRS}/SDL2)
else()
    include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
endif()
include_directories(${CURL_INCLUDE_DIRS})
# Include generated font directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fonts/generated)
# Include src directory for core/logger.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
# Include src/ui directory for UI modules
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
# Include src/api and src/json directories for API modules  
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/json)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
if(EMBEDDED_BUILD)
    # Embedded build - minimal dependencies for SDL2 framebuffer-only + curl
    target_link_libraries(${PROJECT_NAME} 
        panelkit_core
        panelkit_display
        ${PANELKIT_INPUT_LIB}
        ${SDL2_STATIC_LIBRARIES}
        ${SDL2_TTF_STATIC_LIBRARIES}
        ${CURL_STATIC_LIBRARIES}
        # Font and compression
        freetype
        z
        brotlidec
        brotlicommon
        # Network/security (minimal curl dependencies)
        ssl
        crypto
        z
        # System libraries
        m
        pthread
        dl
        rt
    )
    message(STATUS "Linked minimal embedded dependencies")
else()
    # Development build - standard dynamic linking
    target_link_libraries(${PROJECT_NAME} 
        panelkit_core
        panelkit_display
        ${PANELKIT_INPUT_LIB}
        ${SDL2_LIBRARIES} 
        ${SDL2_TTF_LIBRARIES} 
        ${CURL_LIBRARIES} 
        m 
        pthread
    )
    message(STATUS "Linked development dependencies")
endif()