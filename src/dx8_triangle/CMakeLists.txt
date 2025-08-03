add_executable(dx8_triangle main.cpp)

target_link_libraries(dx8_triangle
    dx8gl
    lvgl
    lvgl_platform
    ${GLM_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

target_include_directories(dx8_triangle PRIVATE
    ${CMAKE_SOURCE_DIR}/ext/dx8gl/src
    ${CMAKE_SOURCE_DIR}/ext/glm
    ${CMAKE_SOURCE_DIR}/ext/lvgl
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/lvgl_platform
)