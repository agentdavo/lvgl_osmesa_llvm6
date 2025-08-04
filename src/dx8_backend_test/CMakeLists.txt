# DirectX 8 Backend Test - Test both OSMesa and EGL backends

# Create executable
add_executable(dx8_backend_test main.cpp)

# Link with required libraries
target_link_libraries(dx8_backend_test
    PRIVATE
    lvgl
    dx8gl
    ${CMAKE_DL_LIBS}
)

# Add include directories
target_include_directories(dx8_backend_test PRIVATE
    ${CMAKE_SOURCE_DIR}/ext/dx8gl/src
    ${CMAKE_SOURCE_DIR}/ext/glm
)

# Set output directory
set_target_properties(dx8_backend_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/dx8_backend_test"
)

# Copy any required resources
# (none needed for this test)