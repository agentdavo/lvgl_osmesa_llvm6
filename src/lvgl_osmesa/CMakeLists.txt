cmake_minimum_required(VERSION 3.16)
project(lvgl_osmesa_example)

# Find packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)
pkg_check_modules(GLU REQUIRED glu)

# Create executable
add_executable(lvgl_osmesa main.cpp)

# Find SDL2
find_package(SDL2 REQUIRED)

# Link libraries - order matters for static libraries
target_link_libraries(lvgl_osmesa
    PRIVATE
    lvgl_platform
    lvgl
    ${SDL2_LIBRARIES}
    OSMesa
    ${GLU_LIBRARIES}
    ${X11_LIBRARIES}
)

# Include directories
target_include_directories(lvgl_osmesa
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/lvgl_platform
)

# Set C++ standard
set_target_properties(lvgl_osmesa PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)