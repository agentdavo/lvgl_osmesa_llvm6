add_executable(lvgl_hello main.cpp)
target_include_directories(lvgl_hello PRIVATE
    ${CMAKE_SOURCE_DIR}/src/lvgl_platform)
target_link_libraries(lvgl_hello PRIVATE lvgl_platform lvgl ${SDL2_LIBRARIES})
