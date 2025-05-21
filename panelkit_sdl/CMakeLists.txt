cmake_minimum_required(VERSION 3.10)
project(panelkit C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Debug flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -Wextra")

# Source files
set(SOURCES
    app.c
    api_functions.c
)

# Find SDL2
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
    # Fallback to pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
  endif()
else()
  # For Linux and other platforms
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# Find SDL2_ttf
if(APPLE)
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
    # Fallback to pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
  endif()
else()
  # For Linux and other platforms
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
endif()

# Include directories - special handling for macOS
if(APPLE)
  include_directories(${SDL2_INCLUDE_DIRS}/SDL2 ${SDL2_TTF_INCLUDE_DIRS}/SDL2)
else()
  include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
endif()

# Find libcurl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${CURL_LIBRARIES} m pthread)