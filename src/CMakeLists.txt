# Build lvgl_platform helper library
add_subdirectory(lvgl_platform)

# Build examples
add_subdirectory(lvgl_osmesa)
add_subdirectory(osmesa_test)
add_subdirectory(lvgl_hello)
add_subdirectory(dx8_cube)

# Simple triangle test
add_executable(simple_triangle simple_triangle.cpp)
target_link_libraries(simple_triangle 
    PRIVATE 
    dx8gl
    ${CMAKE_DL_LIBS}
)
target_include_directories(simple_triangle 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/ext/dx8gl/src
)

# GL triangle test
add_executable(gl_triangle_test gl_triangle_test.cpp)
target_link_libraries(gl_triangle_test 
    PRIVATE 
    ${OSMESA_LIB}
    ${CMAKE_DL_LIBS}
)
target_include_directories(gl_triangle_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/build/mesa-install/include
)
target_link_directories(gl_triangle_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/build/llvm-install/lib
)
add_dependencies(gl_triangle_test osmesa)

# DX8 single frame test
add_executable(dx8_single_frame dx8_single_frame.cpp)
target_link_libraries(dx8_single_frame 
    PRIVATE 
    dx8gl
    ${CMAKE_DL_LIBS}
)
target_include_directories(dx8_single_frame 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/ext/dx8gl/src
    ${CMAKE_SOURCE_DIR}/ext/glm
)

# DX8 NDC test
add_executable(dx8_ndc_test dx8_ndc_test.cpp)
target_link_libraries(dx8_ndc_test 
    PRIVATE 
    dx8gl
    ${CMAKE_DL_LIBS}
)
target_include_directories(dx8_ndc_test 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/ext/dx8gl/src
)

# DX8 triangle test subdirectory
add_subdirectory(dx8_triangle)

# DX8 simple test
add_subdirectory(dx8_simple_test)