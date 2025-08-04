cmake_minimum_required(VERSION 3.16)
project(egl_backend_test)

add_executable(egl_backend_test main.cpp)

# Find EGL and OpenGL ES 2.0
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(EGL egl)
    pkg_check_modules(GLES2 glesv2)
endif()

# Link with dx8gl, LVGL, and EGL libraries
target_link_libraries(egl_backend_test
    PRIVATE
        dx8gl
        ${EGL_LIBRARIES}
        ${GLES2_LIBRARIES}
)

target_include_directories(egl_backend_test
    PRIVATE
        ${EGL_INCLUDE_DIRS}
        ${GLES2_INCLUDE_DIRS}
)

# Set output directory
set_target_properties(egl_backend_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/egl_backend_test"
)

# Only build this if EGL is enabled
if(NOT DX8GL_ENABLE_EGL)
    set_target_properties(egl_backend_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()