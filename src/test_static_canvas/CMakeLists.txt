add_executable(test_static_canvas main.cpp)

target_link_libraries(test_static_canvas
    PRIVATE
        lvgl
        lvgl_platform
)