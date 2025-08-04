# Build lvgl_platform helper library
add_subdirectory(lvgl_platform)

# Build examples
add_subdirectory(lvgl_osmesa)
add_subdirectory(dx8_cube)

# Test for XYZRHW vertex crash
add_executable(test_xyzrhw test_xyzrhw.cpp)
target_link_libraries(test_xyzrhw PRIVATE dx8gl lvgl lvgl_platform OSMesa m pthread ${CMAKE_DL_LIBS})
target_include_directories(test_xyzrhw PRIVATE ${CMAKE_SOURCE_DIR}/ext/dx8gl/src)

# Test for mixing regular and XYZRHW vertices
add_executable(test_mixed_vertices test_mixed_vertices.cpp)
target_link_libraries(test_mixed_vertices PRIVATE dx8gl lvgl lvgl_platform OSMesa m pthread ${CMAKE_DL_LIBS})
target_include_directories(test_mixed_vertices PRIVATE ${CMAKE_SOURCE_DIR}/ext/dx8gl/src)

# Test for indexed primitives followed by XYZRHW vertices
add_executable(test_indexed_xyzrhw test_indexed_xyzrhw.cpp)
target_link_libraries(test_indexed_xyzrhw PRIVATE dx8gl lvgl lvgl_platform OSMesa m pthread ${CMAKE_DL_LIBS})
target_include_directories(test_indexed_xyzrhw PRIVATE ${CMAKE_SOURCE_DIR}/ext/dx8gl/src)

# Backend regression test
add_subdirectory(dx8_backend_test)