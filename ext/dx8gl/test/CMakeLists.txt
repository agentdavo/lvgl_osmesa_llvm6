cmake_minimum_required(VERSION 3.10)

# Test executable for shader translator
add_executable(test_shader_translator 
    test_shader_translator.cpp
    ../src/dx8_shader_translator.cpp
    ../src/logger.cpp
)

target_include_directories(test_shader_translator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against OpenGL for GL constants
if(TARGET OSMesa)
    target_link_libraries(test_shader_translator PRIVATE OSMesa)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(test_shader_translator PRIVATE OpenGL::GL)
endif()

# Test executable for texture LOD and dirty regions (simple version)
add_executable(test_texture_simple 
    test_texture_simple.cpp
    ../src/logger.cpp
)

# Link against dx8gl library which contains all needed symbols
target_link_libraries(test_texture_simple PRIVATE dx8gl)

target_include_directories(test_texture_simple PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against OpenGL
if(TARGET OSMesa)
    target_link_libraries(test_texture_simple PRIVATE OSMesa)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(test_texture_simple PRIVATE OpenGL::GL)
endif()

# Test executable for LOD and dirty region logic (no dependencies)
add_executable(test_lod_dirty_logic test_lod_dirty_logic.cpp)

# Test executable for multiple texture coordinates
add_executable(test_multi_texcoords 
    test_multi_texcoords.cpp
    ../src/fvf_utils.cpp
    ../src/logger.cpp
)

target_include_directories(test_multi_texcoords PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against OpenGL for GL constants
if(TARGET OSMesa)
    target_link_libraries(test_multi_texcoords PRIVATE OSMesa)
else()
    target_link_libraries(test_multi_texcoords PRIVATE OpenGL::GL)
endif()

# Test executable for surface format conversions
add_executable(test_surface_format 
    test_surface_format.cpp
    ../src/logger.cpp
)

# Link against dx8gl library
target_link_libraries(test_surface_format PRIVATE dx8gl)

target_include_directories(test_surface_format PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against OpenGL for GL constants
if(TARGET OSMesa)
    target_link_libraries(test_surface_format PRIVATE OSMesa)
else()
    target_link_libraries(test_surface_format PRIVATE OpenGL::GL)
endif()

# Test executable for command buffer async execution
# This test requires the full dx8gl library
add_executable(test_command_buffer_async 
    test_command_buffer_async.cpp
)

target_include_directories(test_command_buffer_async PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against the parent dx8gl library (if available)
if(TARGET dx8gl)
    target_link_libraries(test_command_buffer_async PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_command_buffer_async")
    set_target_properties(test_command_buffer_async PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Simple async test
add_executable(test_async_simple 
    test_async_simple.cpp
)

target_include_directories(test_async_simple PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# WebGPU state mapping test (only if WebGPU is enabled)
if(DX8GL_ENABLE_WEBGPU)
    # Find Google Test
    find_package(GTest QUIET)
    
    if(GTest_FOUND)
        add_executable(test_webgpu_state_mapping
            test_webgpu_state_mapping.cpp
        )
        
        target_include_directories(test_webgpu_state_mapping PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/lib_webgpu
        )
        
        # Link against dx8gl and Google Test
        if(TARGET dx8gl)
            target_link_libraries(test_webgpu_state_mapping PRIVATE dx8gl GTest::gtest GTest::gtest_main)
            add_test(NAME WebGPUStateMappingTest COMMAND test_webgpu_state_mapping)
        else()
            message(WARNING "dx8gl target not available, skipping test_webgpu_state_mapping")
            set_target_properties(test_webgpu_state_mapping PROPERTIES EXCLUDE_FROM_ALL TRUE)
        endif()
    else()
        message(WARNING "Google Test not found, skipping test_webgpu_state_mapping")
    endif()
endif()

if(TARGET dx8gl)
    target_link_libraries(test_async_simple PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_async_simple")
    set_target_properties(test_async_simple PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Device reset test
add_executable(test_device_reset 
    test_device_reset.cpp
)

target_include_directories(test_device_reset PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_device_reset PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_device_reset")
    set_target_properties(test_device_reset PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Cube texture test
add_executable(test_cube_texture 
    test_cube_texture.cpp
)

target_include_directories(test_cube_texture PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_cube_texture PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_cube_texture")
    set_target_properties(test_cube_texture PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Stream source stride test
add_executable(test_stream_source_stride 
    test_stream_source_stride.cpp
)

target_include_directories(test_stream_source_stride PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_stream_source_stride PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_stream_source_stride")
    set_target_properties(test_stream_source_stride PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Shader program cache test
add_executable(test_shader_program_cache 
    test_shader_program_cache.cpp
)

target_include_directories(test_shader_program_cache PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_shader_program_cache PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_shader_program_cache")
    set_target_properties(test_shader_program_cache PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Shader cache resize test
add_executable(test_shader_cache_resize 
    test_shader_cache_resize_simple.cpp
    ../src/shader_binary_cache.cpp
    ../src/logger.cpp
)

target_include_directories(test_shader_cache_resize PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against OpenGL for GL constants
if(TARGET OSMesa)
    target_link_libraries(test_shader_cache_resize PRIVATE OSMesa)
else()
    target_link_libraries(test_shader_cache_resize PRIVATE OpenGL::GL)
endif()

# Vertex shader disassembly test
add_executable(test_vertex_shader_disassembly 
    test_vertex_shader_disassembly.cpp
)

target_include_directories(test_vertex_shader_disassembly PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_vertex_shader_disassembly PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_vertex_shader_disassembly")
    set_target_properties(test_vertex_shader_disassembly PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Core API test
add_executable(test_dx8gl_core_api 
    test_dx8gl_core_api.cpp
)

target_include_directories(test_dx8gl_core_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_dx8gl_core_api PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_dx8gl_core_api")
    set_target_properties(test_dx8gl_core_api PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# D3DX Surface Loading test
add_executable(test_d3dx_surface_loading 
    test_d3dx_surface_loading.cpp
)

target_include_directories(test_d3dx_surface_loading PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_d3dx_surface_loading PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_d3dx_surface_loading")
    set_target_properties(test_d3dx_surface_loading PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Swap Chain Presentation test
add_executable(test_swapchain_presentation 
    test_swapchain_presentation.cpp
)

target_include_directories(test_swapchain_presentation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_swapchain_presentation PRIVATE dx8gl)
else()
    message(WARNING "dx8gl target not available, skipping test_swapchain_presentation")
    set_target_properties(test_swapchain_presentation PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# State Manager Validation test
add_executable(test_state_manager_validation 
    test_state_manager_validation.cpp
    ../src/state_manager.cpp
    ../src/logger.cpp
    gl_stubs.cpp
)

target_include_directories(test_state_manager_validation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against OpenGL for GL constants
if(TARGET OSMesa)
    target_link_libraries(test_state_manager_validation PRIVATE OSMesa)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(test_state_manager_validation PRIVATE OpenGL::GL)
endif()

# Enable testing
enable_testing()
add_test(NAME ShaderTranslatorTest COMMAND test_shader_translator)
add_test(NAME TextureSimpleTest COMMAND test_texture_simple)
add_test(NAME LODDirtyLogicTest COMMAND test_lod_dirty_logic)
add_test(NAME MultiTexCoordsTest COMMAND test_multi_texcoords)
add_test(NAME SurfaceFormatTest COMMAND test_surface_format)
if(TARGET dx8gl)
    add_test(NAME CommandBufferAsyncTest COMMAND test_command_buffer_async)
    add_test(NAME DeviceResetTest COMMAND test_device_reset)
    add_test(NAME CubeTextureTest COMMAND test_cube_texture)
    add_test(NAME StreamSourceStrideTest COMMAND test_stream_source_stride)
    add_test(NAME ShaderProgramCacheTest COMMAND test_shader_program_cache)
    add_test(NAME VertexShaderDisassemblyTest COMMAND test_vertex_shader_disassembly)
    add_test(NAME CoreAPITest COMMAND test_dx8gl_core_api)
    add_test(NAME D3DXSurfaceLoadingTest COMMAND test_d3dx_surface_loading)
    add_test(NAME SwapChainPresentationTest COMMAND test_swapchain_presentation)
endif()
add_test(NAME ShaderCacheResizeTest COMMAND test_shader_cache_resize)
add_test(NAME StateManagerValidationTest COMMAND test_state_manager_validation)

# Backend selection test
add_executable(test_backend_selection 
    test_backend_selection.cpp
)

target_include_directories(test_backend_selection PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_backend_selection PRIVATE dx8gl)
    add_test(NAME BackendSelectionTest COMMAND test_backend_selection)
else()
    message(WARNING "dx8gl target not available, skipping test_backend_selection")
    set_target_properties(test_backend_selection PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Framebuffer correctness test
add_executable(test_framebuffer_correctness 
    test_framebuffer_correctness.cpp
    ../src/offscreen_framebuffer.cpp
    ../src/logger.cpp
)

target_include_directories(test_framebuffer_correctness PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against OpenGL for GL constants and dx8gl
if(TARGET dx8gl)
    target_link_libraries(test_framebuffer_correctness PRIVATE dx8gl)
elseif(TARGET OSMesa)
    target_link_libraries(test_framebuffer_correctness PRIVATE OSMesa)
else()
    target_link_libraries(test_framebuffer_correctness PRIVATE OpenGL::GL)
endif()

add_test(NAME FramebufferCorrectnessTest COMMAND test_framebuffer_correctness)

# HUD font test
add_executable(test_hud_font 
    test_hud_font.cpp
)

target_include_directories(test_hud_font PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_hud_font PRIVATE dx8gl)
    add_test(NAME HUDFontTest COMMAND test_hud_font)
else()
    message(WARNING "dx8gl target not available, skipping test_hud_font")
    set_target_properties(test_hud_font PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Alpha blending test
add_executable(test_alpha_blending 
    test_alpha_blending.cpp
)

target_include_directories(test_alpha_blending PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_alpha_blending PRIVATE dx8gl)
    add_test(NAME AlphaBlendingTest COMMAND test_alpha_blending)
else()
    message(WARNING "dx8gl target not available, skipping test_alpha_blending")
    set_target_properties(test_alpha_blending PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Render states test
add_executable(test_render_states 
    test_render_states.cpp
)

target_include_directories(test_render_states PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_render_states PRIVATE dx8gl)
    add_test(NAME RenderStatesTest COMMAND test_render_states)
else()
    message(WARNING "dx8gl target not available, skipping test_render_states")
    set_target_properties(test_render_states PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()