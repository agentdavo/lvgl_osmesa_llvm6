# samples/CMakeLists.txt

# Enable testing for non-Emscripten builds
if(NOT EMSCRIPTEN)
    enable_testing()
    include(CTest)
endif()

# Helper function to create samples with Emscripten support
function(add_dx8gl_sample name source)
    add_executable(dx8gl_${name} ${source})
    
    # Check if sample uses SDL
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/${source} SOURCE_CONTENT)
    if(SOURCE_CONTENT MATCHES "SDL")
        target_link_libraries(dx8gl_${name} PRIVATE dx8gl SDL3-static)
        target_include_directories(dx8gl_${name} PRIVATE ${CMAKE_SOURCE_DIR}/lib/SDL/include)
    else()
        target_link_libraries(dx8gl_${name} PRIVATE dx8gl)
    endif()

    if(EMSCRIPTEN)
        # Select shell file based on target
        if(${name} STREQUAL "spinning_cubes")
            set(SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell_fullscreen.html")
        else()
            set(SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell.html")
        endif()

        # Common linker options for Emscripten
        set(BASE_EXPORT_OPTIONS
            "-sWASM=1"
            "-sUSE_WEBGL2=1"
            "-sMIN_WEBGL_VERSION=2"
            "-sMAX_WEBGL_VERSION=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
            "-sNO_EXIT_RUNTIME=1"
        )
		
        # Debug vs. Release configurations
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(EXPORT_OPTIONS
                "${BASE_EXPORT_OPTIONS}"
                "-sASSERTIONS=2"
                "-gsource-map"
                "-O0"
                "-sSTACK_OVERFLOW_CHECK=2"
            )
        else()
            set(EXPORT_OPTIONS
                "${BASE_EXPORT_OPTIONS}"
                "-O3"
                "-sASYNCIFY"
            )
        endif()

        # Convert list to space-separated string
        string(JOIN " " EXPORT_OPTIONS_STR ${EXPORT_OPTIONS})
        set_target_properties(dx8gl_${name} PROPERTIES
            SUFFIX ".html"
            LINK_FLAGS "${EXPORT_OPTIONS_STR} --shell-file ${SHELL_FILE}"
        )
    else()
        # Desktop: Add as a test for CTest
        add_test(NAME dx8gl_${name}
            COMMAND dx8gl_${name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        set_tests_properties(dx8gl_${name} PROPERTIES
            TIMEOUT 30
            LABELS "dx8gl;samples"
            ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib/dx8gl:$ENV{LD_LIBRARY_PATH}"
        )
        # Check for PPM/PNG output
        if(${name} MATCHES "clear_test|triangle|textured_cube|lighting_test")
            set_tests_properties(dx8gl_${name} PROPERTIES
                PASS_REGULAR_EXPRESSION "Saved.*\\.ppm|Test completed successfully"
            )
        elseif(${name} MATCHES "triangle_with_hud")
            set_tests_properties(dx8gl_${name} PROPERTIES
                PASS_REGULAR_EXPRESSION "Saved.*\\.png|Test completed successfully"
            )
        endif()
    endif()
endfunction()

# Define sample applications
set(SAMPLE_APPS
    # These are samples in the current directory
)

foreach(name ${SAMPLE_APPS})
    add_dx8gl_sample(${name} ${name}.cpp)
endforeach()

# Add webgpu-demo from subdirectory
add_executable(dx8gl_webgpu-demo webgpu-demo/dx8_webgpu_demo.cpp)
target_link_libraries(dx8gl_webgpu-demo PRIVATE dx8gl)

if(EMSCRIPTEN)
    set(SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell_fullscreen.html")
    set(BASE_EXPORT_OPTIONS
        "-sWASM=1"
        "-sUSE_WEBGL2=1"
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "-sNO_EXIT_RUNTIME=1"
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(EXPORT_OPTIONS
            "${BASE_EXPORT_OPTIONS}"
            "-sASSERTIONS=2"
            "-gsource-map"
            "-O0"
            "-sSTACK_OVERFLOW_CHECK=2"
        )
    else()
        set(EXPORT_OPTIONS
            "${BASE_EXPORT_OPTIONS}"
            "-O3"
            "-sASYNCIFY"
        )
    endif()
    
    string(JOIN " " EXPORT_OPTIONS_STR ${EXPORT_OPTIONS})
    set_target_properties(dx8gl_webgpu-demo PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "${EXPORT_OPTIONS_STR} --shell-file ${SHELL_FILE}"
    )
else()
    add_test(NAME dx8gl_webgpu-demo
        COMMAND dx8gl_webgpu-demo
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(dx8gl_webgpu-demo PROPERTIES
        TIMEOUT 30
        LABELS "dx8gl;samples"
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib/dx8gl:$ENV{LD_LIBRARY_PATH}"
    )
endif()

# Add WebGPU sample if WebGPU backend is enabled
if(DX8GL_ENABLE_WEBGPU)
    add_dx8gl_sample(webgpu_complex_scene webgpu_complex_scene.cpp)
    
    # WebGPU sample needs special configuration for Emscripten
    if(EMSCRIPTEN)
        # Add WebGPU-specific flags
        get_target_property(CURRENT_LINK_FLAGS dx8gl_webgpu_complex_scene LINK_FLAGS)
        set_target_properties(dx8gl_webgpu_complex_scene PROPERTIES
            LINK_FLAGS "${CURRENT_LINK_FLAGS} -sUSE_WEBGPU=1 -sOFFSCREENCANVAS_SUPPORT=1"
        )
        
        # Add threading flags based on configuration
        if(DX8GL_WEBGPU_THREADING STREQUAL "pthreads")
            set_target_properties(dx8gl_webgpu_complex_scene PROPERTIES
                LINK_FLAGS "${CURRENT_LINK_FLAGS} -pthread -sPROXY_TO_PTHREAD=1"
            )
        elseif(DX8GL_WEBGPU_THREADING STREQUAL "wasm-workers")
            set_target_properties(dx8gl_webgpu_complex_scene PROPERTIES
                LINK_FLAGS "${CURRENT_LINK_FLAGS} -sWASM_WORKERS=1"
            )
        endif()
    endif()
endif()