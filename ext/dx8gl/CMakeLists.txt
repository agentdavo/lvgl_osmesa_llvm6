cmake_minimum_required(VERSION 3.16)

project(dx8gl VERSION 1.0.0 LANGUAGES C CXX)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

# C++20 and C11 standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Prevent shared builds
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(SDL_SHARED OFF CACHE BOOL "Build SDL as shared library" FORCE)
set(SDL_STATIC  ON CACHE BOOL "Build SDL as static library" FORCE)

# Find system libraries for non-Emscripten platforms
if(NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    
    # We're using OSMesa from the parent project
    # No need to find OpenGL packages - OSMesa provides everything
    set(GL_LIBRARIES "")
    set(GL_INCLUDE_DIRS "")
    add_compile_definitions(DX8GL_USE_DESKTOP_GL)
    
    # Optional EGL support
    option(DX8GL_ENABLE_EGL "Enable EGL backend support" OFF)
    if(DX8GL_ENABLE_EGL)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(EGL egl)
            pkg_check_modules(GLES3 glesv2)
        endif()
        
        if(EGL_FOUND AND GLES3_FOUND)
            message(STATUS "EGL backend support enabled")
            add_compile_definitions(DX8GL_HAS_EGL)
            set(EGL_LIBRARIES ${EGL_LIBRARIES} ${GLES3_LIBRARIES})
            set(EGL_INCLUDE_DIRS ${EGL_INCLUDE_DIRS} ${GLES3_INCLUDE_DIRS})
        else()
            message(WARNING "EGL or GLESv2 not found, EGL backend disabled")
            set(DX8GL_ENABLE_EGL OFF)
        endif()
    endif()
    
    # Optional WebGPU support
    option(DX8GL_ENABLE_WEBGPU "Enable WebGPU backend support" OFF)
    if(DX8GL_ENABLE_WEBGPU)
        message(STATUS "WebGPU backend support enabled")
        add_compile_definitions(DX8GL_HAS_WEBGPU)
        
        # WebGPU threading mode options
        set(DX8GL_WEBGPU_THREADING "none" CACHE STRING "WebGPU threading mode: none, pthreads, wasm-workers")
        set_property(CACHE DX8GL_WEBGPU_THREADING PROPERTY STRINGS none pthreads wasm-workers)
        
        # Find or configure WebGPU implementation
        option(DX8GL_WEBGPU_USE_DAWN "Use Dawn for native WebGPU" OFF)
        option(DX8GL_WEBGPU_USE_WGPU "Use wgpu-native for native WebGPU" OFF)
        
        if(NOT EMSCRIPTEN)
            # Native WebGPU implementation
            if(DX8GL_WEBGPU_USE_DAWN)
                find_package(Dawn QUIET)
                if(Dawn_FOUND)
                    message(STATUS "Using Dawn for WebGPU backend")
                    add_compile_definitions(DX8GL_WEBGPU_USE_DAWN)
                else()
                    message(WARNING "Dawn not found, WebGPU backend disabled")
                    set(DX8GL_ENABLE_WEBGPU OFF)
                endif()
            elseif(DX8GL_WEBGPU_USE_WGPU)
                find_package(wgpu QUIET)
                if(wgpu_FOUND)
                    message(STATUS "Using wgpu-native for WebGPU backend")
                    add_compile_definitions(DX8GL_WEBGPU_USE_WGPU)
                else()
                    message(WARNING "wgpu-native not found, WebGPU backend disabled")
                    set(DX8GL_ENABLE_WEBGPU OFF)
                endif()
            else()
                message(WARNING "No native WebGPU implementation selected (Dawn or wgpu-native)")
                set(DX8GL_ENABLE_WEBGPU OFF)
            endif()
        endif()
    endif()
    
    # Always use OSMesa from parent project
    if(TARGET OSMesa)
        message(STATUS "Using OSMesa from parent project")
        add_compile_definitions(DX8GL_HAS_OSMESA)
    else()
        message(FATAL_ERROR "OSMesa target not found from parent project. Build must be run from main project, not standalone.")
    endif()
else()
    set(GL_INCLUDE_DIRS "")
    set(GL_LIBRARIES "")
    
    # Base Emscripten linker flags
    set(EMSCRIPTEN_LINK_FLAGS
        "-sUSE_WEBGL2=1"
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sENVIRONMENT=web"
        "-sEXPORTED_FUNCTIONS=['_main']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    )
    
    # WebGPU support for Emscripten
    if(DX8GL_ENABLE_WEBGPU)
        message(STATUS "Configuring Emscripten for WebGPU")
        add_compile_definitions(DX8GL_HAS_WEBGPU)
        list(APPEND EMSCRIPTEN_LINK_FLAGS "-sUSE_WEBGPU=1")
        
        # Threading mode configuration
        if(DX8GL_WEBGPU_THREADING STREQUAL "pthreads")
            message(STATUS "WebGPU threading: pthreads")
            add_compile_definitions(__EMSCRIPTEN_PTHREADS__)
            list(APPEND EMSCRIPTEN_LINK_FLAGS 
                "-pthread"
                "-sPROXY_TO_PTHREAD=1"
                "-sPTHREAD_POOL_SIZE=4"
            )
        elseif(DX8GL_WEBGPU_THREADING STREQUAL "wasm-workers")
            message(STATUS "WebGPU threading: Wasm Workers")
            add_compile_definitions(__EMSCRIPTEN_WASM_WORKERS__)
            list(APPEND EMSCRIPTEN_LINK_FLAGS
                "-sWASM_WORKERS=1"
                "-sWORKER_POOL_SIZE=4"
            )
        else()
            message(STATUS "WebGPU threading: none (main thread)")
        endif()
        
        # OffscreenCanvas support
        list(APPEND EMSCRIPTEN_LINK_FLAGS "-sOFFSCREENCANVAS_SUPPORT=1")
    endif()
    
    # Apply all link options
    foreach(flag ${EMSCRIPTEN_LINK_FLAGS})
        add_link_options(${flag})
    endforeach()
    
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# OSMesa-only mode - no SDL3 required

# dx8gl library sources
set(DX8GL_SOURCES
    src/dx8gl.cpp
    src/d3d8_interface.cpp
    src/d3d8_device.cpp
    src/d3d8_texture.cpp
    src/d3d8_cubetexture.cpp
    src/cube_texture_support.cpp
    src/d3d8_volumetexture.cpp
    src/d3d8_volume.cpp
    src/d3d8_vertexbuffer.cpp
    src/d3d8_indexbuffer.cpp
    src/d3d8_surface.cpp
    src/d3d8_swapchain.cpp
    src/d3d8_com_wrapper.cpp
    src/shader_generator.cpp
    src/state_manager.cpp
    src/command_buffer.cpp
    src/resource_pool.cpp
    src/resource_pool_integration.cpp
    src/thread_pool.cpp
    src/render_thread.cpp
    src/logger.cpp
    src/vao_manager.cpp
    src/d3dx_shader.cpp
    src/d3dx_texture.cpp
    src/d3dx_math.cpp
    src/dx8_shader_translator.cpp
    src/vertex_shader_manager.cpp
    src/pixel_shader_manager.cpp
    src/shader_bytecode_assembler.cpp
    src/shader_bytecode_disassembler.cpp
    src/shader_constant_manager.cpp
    src/shader_debugger.cpp
    src/shader_binary_cache.cpp
    src/shader_hot_reload.cpp
    src/texcoord_generator.cpp
    src/hud_system.cpp
    src/fixed_function_shader.cpp
    src/fvf_utils.cpp
    src/private_data.cpp
    src/shader_program_manager.cpp
    src/d3d8_missing_stubs.cpp
    src/render_backend.cpp
    src/offscreen_framebuffer.cpp
)

# Always add OSMesa context implementation
list(APPEND DX8GL_SOURCES src/osmesa_context.cpp)
list(APPEND DX8GL_SOURCES src/osmesa_gl_loader.cpp)
list(APPEND DX8GL_SOURCES src/osmesa_backend.cpp)

# Conditionally add EGL backend
if(DX8GL_ENABLE_EGL)
    list(APPEND DX8GL_SOURCES src/egl_backend.cpp)
endif()

# Conditionally add WebGPU backend
if(DX8GL_ENABLE_WEBGPU)
    list(APPEND DX8GL_SOURCES src/webgpu_backend.cpp)
    list(APPEND DX8GL_SOURCES src/webgpu_async_handler.cpp)
    list(APPEND DX8GL_SOURCES src/webgpu_state_mapper.cpp)
    list(APPEND DX8GL_SOURCES src/wgsl_shader_translator.cpp)
    list(APPEND DX8GL_SOURCES src/wgsl_shader_cache.cpp)
    list(APPEND DX8GL_SOURCES lib/lib_webgpu/lib_webgpu.cpp)
    list(APPEND DX8GL_SOURCES lib/lib_webgpu/offscreen_canvas.c)
endif()

# dx8gl now uses OSMesa-only for software rendering (no EGL complexity)
message(STATUS "dx8gl configured for OSMesa-only software rendering")

# Create dx8gl static library
add_library(dx8gl STATIC ${DX8GL_SOURCES})

# Configure include directories
target_include_directories(dx8gl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
    ${GL_INCLUDE_DIRS}
)
target_include_directories(dx8gl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm
)

# Add EGL include directories if enabled
if(DX8GL_ENABLE_EGL)
    target_include_directories(dx8gl PRIVATE ${EGL_INCLUDE_DIRS})
endif()

# Add WebGPU include directories if enabled
if(DX8GL_ENABLE_WEBGPU)
    target_include_directories(dx8gl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/lib_webgpu)
endif()

# Add OSMesa include directory
target_include_directories(dx8gl PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../osmesa/include")

# Link libraries
if(NOT EMSCRIPTEN)
    target_link_libraries(dx8gl 
        PUBLIC
            ${GL_LIBRARIES}
            Threads::Threads
    )
    
    # Add OSMesa if found
    if(OSMESA_LIBRARY)
        target_link_libraries(dx8gl PUBLIC ${OSMESA_LIBRARY})
    endif()
    
    # Add EGL libraries if enabled
    if(DX8GL_ENABLE_EGL)
        target_link_libraries(dx8gl PUBLIC ${EGL_LIBRARIES})
    endif()
    
    # Add WebGPU libraries if enabled
    if(DX8GL_ENABLE_WEBGPU)
        if(DX8GL_WEBGPU_USE_DAWN AND Dawn_FOUND)
            target_link_libraries(dx8gl PUBLIC Dawn::dawn_native Dawn::dawn_platform)
        elseif(DX8GL_WEBGPU_USE_WGPU AND wgpu_FOUND)
            target_link_libraries(dx8gl PUBLIC wgpu::wgpu)
        endif()
    endif()
else()
    # Emscripten only needs GLES2 (WebGPU is handled by browser)
    target_link_libraries(dx8gl PUBLIC ${GL_LIBRARIES})
endif()

# Always use OSMesa from local lib/ folder - no fallbacks
target_compile_definitions(dx8gl PRIVATE DX8GL_HAS_OSMESA)

# For C++17 compatibility with game code that includes d3d8_game.h
target_compile_definitions(dx8gl PRIVATE DX8GL_USE_CPP_INTERFACES)

# Only use OSMesa from parent project - no system fallbacks
if(TARGET OSMesa)
    target_link_libraries(dx8gl PRIVATE OSMesa)
    message(STATUS "OSMesa support enabled (using local lib/ OSMesa)")
else()
    message(FATAL_ERROR "OSMesa target not found from parent project lib/ folder. dx8gl must be built as part of the main project.")
endif()

# Compile definitions
target_compile_definitions(dx8gl PRIVATE
    DX8GL_BUILDING_DLL
    GL_GLEXT_PROTOTYPES
    SDL_MAIN_USE_CALLBACKS
)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT EMSCRIPTEN)
    target_compile_options(dx8gl PRIVATE
        -Wall -Wextra -Wpedantic
        -O3 -march=native
        -ffast-math
        -funroll-loops
        -ftree-vectorize
    )
elseif(EMSCRIPTEN)
    target_compile_options(dx8gl PRIVATE
        -Wall -Wextra
        -O3
        -ffast-math
    )
endif()

# Sample application
option(BUILD_DX8GL_SAMPLES "Build dx8gl sample applications" OFF)
if(BUILD_DX8GL_SAMPLES)
    add_subdirectory(samples)
endif()

# Tests
option(BUILD_DX8GL_TESTS "Build dx8gl unit tests" ON)
if(BUILD_DX8GL_TESTS)
    add_subdirectory(test)
endif()