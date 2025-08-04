add_executable(test_lvgl_platform main.cpp)

target_link_libraries(test_lvgl_platform
    PRIVATE
        lvgl
        lvgl_platform
)