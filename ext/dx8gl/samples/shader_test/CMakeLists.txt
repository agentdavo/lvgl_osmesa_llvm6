# Shader translation and cache test

# Create executable
add_executable(dx8gl_shader_test shader_test.cpp)

# Link with dx8gl
target_link_libraries(dx8gl_shader_test PRIVATE dx8gl)

# Include directories
target_include_directories(dx8gl_shader_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(dx8gl_shader_test PRIVATE opengl32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(dx8gl_shader_test PRIVATE GL)
elseif(APPLE)
    find_library(OPENGL_LIBRARY OpenGL)
    target_link_libraries(dx8gl_shader_test PRIVATE ${OPENGL_LIBRARY})
endif()

# Set C++ standard
set_target_properties(dx8gl_shader_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add as test for CTest
if(NOT EMSCRIPTEN)
    add_test(NAME dx8gl_shader_translation
        COMMAND dx8gl_shader_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(dx8gl_shader_translation PROPERTIES
        TIMEOUT 30
        LABELS "dx8gl;shader;translation;cache"
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib/dx8gl:$ENV{LD_LIBRARY_PATH}"
        PASS_REGULAR_EXPRESSION "Failed: 0"
    )
endif()