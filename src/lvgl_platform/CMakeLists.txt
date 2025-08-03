add_library(lvgl_platform STATIC lvgl_platform.cpp)

target_include_directories(lvgl_platform 
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/src/lvgl_platform
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ext/lvgl
)

target_link_libraries(lvgl_platform PUBLIC lvgl ${SDL2_LIBRARIES})

# Set C++ standard
set_target_properties(lvgl_platform PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)