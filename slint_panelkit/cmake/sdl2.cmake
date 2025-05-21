# SDL2 setup for cross-platform development

# Find SDL2 package
if(APPLE)
    # Use brew's SDL2 on macOS
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
else()
    # For Linux and cross-compilation
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
endif()