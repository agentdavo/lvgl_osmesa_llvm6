cmake_minimum_required(VERSION 3.16)
project(osmesa_test)

# Find packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLU REQUIRED glu)

# Create executable
add_executable(osmesa_test main.cpp)

# Link libraries
target_link_libraries(osmesa_test
    PRIVATE
    OSMesa
    ${GLU_LIBRARIES}
)

# Set C++ standard
set_target_properties(osmesa_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)