# Device reset sample

# Create executable
add_executable(dx8gl_device_reset device_reset_test.cpp)

# Link with dx8gl
target_link_libraries(dx8gl_device_reset PRIVATE dx8gl)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(dx8gl_device_reset PRIVATE opengl32 gdi32 user32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(dx8gl_device_reset PRIVATE GL X11)
elseif(APPLE)
    find_library(OPENGL_LIBRARY OpenGL)
    target_link_libraries(dx8gl_device_reset PRIVATE ${OPENGL_LIBRARY})
endif()

# Set C++ standard
set_target_properties(dx8gl_device_reset PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add as test for CTest
if(NOT EMSCRIPTEN)
    add_test(NAME dx8gl_device_reset
        COMMAND dx8gl_device_reset
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(dx8gl_device_reset PROPERTIES
        TIMEOUT 30
        LABELS "dx8gl;samples;device_reset"
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib/dx8gl:$ENV{LD_LIBRARY_PATH}"
        PASS_REGULAR_EXPRESSION "Device reset successful|Test completed successfully"
    )
endif()