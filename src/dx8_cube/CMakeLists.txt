add_executable(dx8_cube main.cpp)

target_link_libraries(dx8_cube
    PRIVATE
        lvgl
        lvgl_platform
        dx8gl
        OSMesa
        ${CMAKE_THREAD_LIBS_INIT}
)

target_include_directories(dx8_cube
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/lvgl_platform
        ${CMAKE_SOURCE_DIR}/ext/dx8gl/src
        ${CMAKE_SOURCE_DIR}/src/lvgl_platform
        ${CMAKE_SOURCE_DIR}/ext/glm
)

# Set RPATH for the executable to find shared libraries
set_target_properties(dx8_cube PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Install the executable
install(TARGETS dx8_cube
    RUNTIME DESTINATION bin
)