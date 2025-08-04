add_executable(test_lvgl_canvas main.cpp)

target_link_libraries(test_lvgl_canvas
    PRIVATE
        lvgl
        lvgl_platform
)