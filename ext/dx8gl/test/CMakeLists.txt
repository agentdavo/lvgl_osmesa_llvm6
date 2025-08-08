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

# COM Wrapper threading test
add_executable(test_com_wrapper_threading 
    test_com_wrapper_threading.cpp
)

target_include_directories(test_com_wrapper_threading PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_com_wrapper_threading PRIVATE dx8gl pthread)
    add_test(NAME COMWrapperThreadingTest COMMAND test_com_wrapper_threading)
    set_tests_properties(COMWrapperThreadingTest PROPERTIES TIMEOUT 60)
else()
    message(WARNING "dx8gl target not available, skipping test_com_wrapper_threading")
    set_target_properties(test_com_wrapper_threading PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for matrix pipeline
add_executable(test_matrix_pipeline
    test_matrix_pipeline.cpp
)

target_include_directories(test_matrix_pipeline PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_matrix_pipeline PRIVATE dx8gl gtest gtest_main)
    add_test(NAME MatrixPipelineTest COMMAND test_matrix_pipeline)
    set_tests_properties(MatrixPipelineTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_matrix_pipeline")
    set_target_properties(test_matrix_pipeline PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for D3DX math functions
add_executable(test_d3dx_math
    test_d3dx_math.cpp
)

target_include_directories(test_d3dx_math PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_d3dx_math PRIVATE dx8gl gtest gtest_main)
    add_test(NAME D3DXMathTest COMMAND test_d3dx_math)
    set_tests_properties(D3DXMathTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_d3dx_math")
    set_target_properties(test_d3dx_math PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for shader constants
add_executable(test_shader_constants
    test_shader_constants.cpp
)

target_include_directories(test_shader_constants PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_shader_constants PRIVATE dx8gl gtest gtest_main)
    add_test(NAME ShaderConstantsTest COMMAND test_shader_constants)
    set_tests_properties(ShaderConstantsTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_shader_constants")
    set_target_properties(test_shader_constants PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for volume textures
add_executable(test_volume_texture
    test_volume_texture.cpp
)

target_include_directories(test_volume_texture PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_volume_texture PRIVATE dx8gl gtest gtest_main)
    add_test(NAME VolumeTextureTest COMMAND test_volume_texture)
    set_tests_properties(VolumeTextureTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_volume_texture")
    set_target_properties(test_volume_texture PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for shader hot reload
add_executable(test_shader_hot_reload
    test_shader_hot_reload.cpp
)

target_include_directories(test_shader_hot_reload PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_shader_hot_reload PRIVATE dx8gl gtest gtest_main stdc++fs pthread)
    add_test(NAME ShaderHotReloadTest COMMAND test_shader_hot_reload)
    set_tests_properties(ShaderHotReloadTest PROPERTIES TIMEOUT 60)
else()
    message(WARNING "dx8gl target not available, skipping test_shader_hot_reload")
    set_target_properties(test_shader_hot_reload PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for D3DX color functions
add_executable(test_d3dx_color
    test_d3dx_color.cpp
)

target_include_directories(test_d3dx_color PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_d3dx_color PRIVATE dx8gl gtest gtest_main)
    add_test(NAME D3DXColorTest COMMAND test_d3dx_color)
    set_tests_properties(D3DXColorTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_d3dx_color")
    set_target_properties(test_d3dx_color PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for shader program linking
add_executable(test_shader_program_linking
    test_shader_program_linking.cpp
)

target_include_directories(test_shader_program_linking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_shader_program_linking PRIVATE dx8gl gtest gtest_main)
    add_test(NAME ShaderProgramLinkingTest COMMAND test_shader_program_linking)
    set_tests_properties(ShaderProgramLinkingTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_shader_program_linking")
    set_target_properties(test_shader_program_linking PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for shader constant batching
add_executable(test_shader_constant_batching
    test_shader_constant_batching.cpp
)

target_include_directories(test_shader_constant_batching PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_shader_constant_batching PRIVATE dx8gl gtest gtest_main)
    add_test(NAME ShaderConstantBatchingTest COMMAND test_shader_constant_batching)
    set_tests_properties(ShaderConstantBatchingTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_shader_constant_batching")
    set_target_properties(test_shader_constant_batching PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for shader cache (simple version without OpenGL)
add_executable(test_shader_cache_simple
    test_shader_cache_simple.cpp
)

target_include_directories(test_shader_cache_simple PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_shader_cache_simple PRIVATE dx8gl gtest gtest_main)
    add_test(NAME ShaderCacheSimpleTest COMMAND test_shader_cache_simple)
    set_tests_properties(ShaderCacheSimpleTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_shader_cache_simple")
    set_target_properties(test_shader_cache_simple PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for device capabilities parity
add_executable(test_device_caps_parity
    test_device_caps_parity.cpp
)

target_include_directories(test_device_caps_parity PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_device_caps_parity PRIVATE dx8gl gtest gtest_main)
    add_test(NAME DeviceCapsParityTest COMMAND test_device_caps_parity)
    set_tests_properties(DeviceCapsParityTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_device_caps_parity")
    set_target_properties(test_device_caps_parity PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for cross-backend rendering
add_executable(test_cross_backend_rendering
    test_cross_backend_rendering.cpp
)

target_include_directories(test_cross_backend_rendering PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_cross_backend_rendering PRIVATE dx8gl gtest gtest_main)
    add_test(NAME CrossBackendRenderingTest COMMAND test_cross_backend_rendering)
    set_tests_properties(CrossBackendRenderingTest PROPERTIES TIMEOUT 60)
else()
    message(WARNING "dx8gl target not available, skipping test_cross_backend_rendering")
    set_target_properties(test_cross_backend_rendering PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for enhanced cross-backend rendering
add_executable(test_cross_backend_enhanced
    test_cross_backend_enhanced.cpp
)

target_include_directories(test_cross_backend_enhanced PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_cross_backend_enhanced PRIVATE dx8gl gtest gtest_main)
    add_test(NAME CrossBackendEnhancedTest COMMAND test_cross_backend_enhanced)
    set_tests_properties(CrossBackendEnhancedTest PROPERTIES TIMEOUT 60)
else()
    message(WARNING "dx8gl target not available, skipping test_cross_backend_enhanced")
    set_target_properties(test_cross_backend_enhanced PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for WebGPU async behavior (if WebGPU is enabled)
if(DX8GL_ENABLE_WEBGPU)
    add_executable(test_webgpu_async
        test_webgpu_async.cpp
    )
    
    target_include_directories(test_webgpu_async PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    
    target_compile_definitions(test_webgpu_async PRIVATE
        DX8GL_HAS_WEBGPU=1
    )
    
    if(TARGET dx8gl)
        target_link_libraries(test_webgpu_async PRIVATE dx8gl gtest gtest_main pthread)
        add_test(NAME WebGPUAsyncTest COMMAND test_webgpu_async)
        set_tests_properties(WebGPUAsyncTest PROPERTIES TIMEOUT 60)
    else()
        message(WARNING "dx8gl target not available, skipping test_webgpu_async")
        set_target_properties(test_webgpu_async PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endif()

# Test executable for cursor management
add_executable(test_cursor_management
    test_cursor_management.cpp
)

target_include_directories(test_cursor_management PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_cursor_management PRIVATE dx8gl gtest gtest_main)
    add_test(NAME CursorManagementTest COMMAND test_cursor_management)
    set_tests_properties(CursorManagementTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_cursor_management")
    set_target_properties(test_cursor_management PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for additional swap chains
add_executable(test_additional_swapchain
    test_additional_swapchain.cpp
)

target_include_directories(test_additional_swapchain PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_additional_swapchain PRIVATE dx8gl gtest gtest_main)
    add_test(NAME AdditionalSwapChainTest COMMAND test_additional_swapchain)
    set_tests_properties(AdditionalSwapChainTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_additional_swapchain")
    set_target_properties(test_additional_swapchain PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for matrix multiplication
add_executable(test_matrix_multiplication
    test_matrix_multiplication.cpp
)

target_include_directories(test_matrix_multiplication PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_matrix_multiplication PRIVATE dx8gl gtest gtest_main)
    add_test(NAME MatrixMultiplicationTest COMMAND test_matrix_multiplication)
    set_tests_properties(MatrixMultiplicationTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_matrix_multiplication")
    set_target_properties(test_matrix_multiplication PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for state blocks
add_executable(test_state_blocks
    test_state_blocks.cpp
)

target_include_directories(test_state_blocks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_state_blocks PRIVATE dx8gl gtest gtest_main)
    add_test(NAME StateBlocksTest COMMAND test_state_blocks)
    set_tests_properties(StateBlocksTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_state_blocks")
    set_target_properties(test_state_blocks PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for clip status
add_executable(test_clip_status
    test_clip_status.cpp
)

target_include_directories(test_clip_status PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_clip_status PRIVATE dx8gl gtest gtest_main)
    add_test(NAME ClipStatusTest COMMAND test_clip_status)
    set_tests_properties(ClipStatusTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_clip_status")
    set_target_properties(test_clip_status PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for palette and device info
add_executable(test_palette_and_device_info
    test_palette_and_device_info.cpp
)

target_include_directories(test_palette_and_device_info PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_palette_and_device_info PRIVATE dx8gl gtest gtest_main)
    add_test(NAME PaletteAndDeviceInfoTest COMMAND test_palette_and_device_info)
    set_tests_properties(PaletteAndDeviceInfoTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_palette_and_device_info")
    set_target_properties(test_palette_and_device_info PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for volume texture and front buffer
add_executable(test_volume_texture_and_front_buffer
    test_volume_texture_and_front_buffer.cpp
)

target_include_directories(test_volume_texture_and_front_buffer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_volume_texture_and_front_buffer PRIVATE dx8gl gtest gtest_main)
    add_test(NAME VolumeTextureAndFrontBufferTest COMMAND test_volume_texture_and_front_buffer)
    set_tests_properties(VolumeTextureAndFrontBufferTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_volume_texture_and_front_buffer")
    set_target_properties(test_volume_texture_and_front_buffer PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for complete COM wrapper
add_executable(test_com_wrapper_complete
    test_com_wrapper_complete.cpp
)

target_include_directories(test_com_wrapper_complete PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_com_wrapper_complete PRIVATE dx8gl gtest gtest_main pthread)
    add_test(NAME COMWrapperCompleteTest COMMAND test_com_wrapper_complete)
    set_tests_properties(COMWrapperCompleteTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_com_wrapper_complete")
    set_target_properties(test_com_wrapper_complete PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Test executable for multisampling
add_executable(test_multisampling
    test_multisampling.cpp
)

target_include_directories(test_multisampling PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_multisampling PRIVATE dx8gl gtest gtest_main)
    add_test(NAME MultisamplingTest COMMAND test_multisampling)
    set_tests_properties(MultisamplingTest PROPERTIES TIMEOUT 30)
else()
    message(WARNING "dx8gl target not available, skipping test_multisampling")
    set_target_properties(test_multisampling PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()