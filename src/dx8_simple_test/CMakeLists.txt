add_executable(dx8_simple_test main.cpp)

target_link_libraries(dx8_simple_test
    dx8gl
    ${CMAKE_DL_LIBS}
)

target_include_directories(dx8_simple_test PRIVATE
    ${CMAKE_SOURCE_DIR}/ext/dx8gl/src
)