cmake_minimum_required(VERSION 3.20)
project(lvgl_osmesa_llvm C CXX)

# Enforce C++ standard for compatibility
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to enable relocatable build
option(BUILD_RELOCATABLE "Build with relocatable paths" ON)

# Detect architecture for library paths
set(LIB_ARCH_DIR "${CMAKE_LIBRARY_ARCHITECTURE}")
if(NOT LIB_ARCH_DIR)
    set(LIB_ARCH_DIR "x86_64-linux-gnu") # Fallback for Linux
endif()

# Configure RPATH for relocatable binaries
if(BUILD_RELOCATABLE)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:$ORIGIN/../../llvm-install/lib:$ORIGIN/../../mesa-install/lib/${LIB_ARCH_DIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

    # For macOS
    if(APPLE)
        set(CMAKE_MACOSX_RPATH TRUE)
        set(CMAKE_INSTALL_NAME_DIR "@executable_path/../lib")
    endif()
endif()

# Define paths for external projects
set(LLVM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/llvm-project/llvm)
set(LLVM_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-build)
set(LLVM_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-install)
set(LLVM_CONFIG ${LLVM_INSTALL_DIR}/bin/llvm-config)
set(MESA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/mesa)
set(MESA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mesa-build)
set(MESA_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/mesa-install)
set(OSMESA_LIB "${MESA_INSTALL_DIR}/lib/${LIB_ARCH_DIR}/libOSMesa.so")

include(ExternalProject)

# LVGL Display Backend Options
option(LVGL_USE_X11 "Enable LVGL X11 backend" ON)
option(LVGL_USE_SDL "Enable LVGL SDL backend" ON)
option(LVGL_USE_WAYLAND "Enable LVGL Wayland backend" OFF)
option(LVGL_USE_LINUX_DRM "Enable LVGL DRM backend" OFF)
option(LVGL_USE_LINUX_FBDEV "Enable LVGL FBDev backend" OFF)
option(LVGL_USE_NUTTX "Enable LVGL NuttX backend" OFF)
option(LVGL_INCLUDE_DEMOS "Include LVGL demo sources" OFF)

# dx8gl Backend Options
option(DX8GL_ENABLE_EGL "Enable EGL backend for dx8gl" OFF)

# LLVM External Project
ExternalProject_Add(llvm_external
    SOURCE_DIR ${LLVM_SOURCE_DIR}
    BINARY_DIR ${LLVM_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=MinSizeRel
        -DCMAKE_INSTALL_PREFIX=${LLVM_INSTALL_DIR}
        -DCMAKE_INSTALL_RPATH=$ORIGIN/../lib
        -DLLVM_APPEND_VC_REV=OFF
        -DLLVM_TARGETS_TO_BUILD=X86
        -DLLVM_BUILD_LLVM_DYLIB=ON
        -DLLVM_LINK_LLVM_DYLIB=ON
        -DLLVM_DYLIB_COMPONENTS=all
        -DLLVM_INSTALL_UTILS=ON
        -DLLVM_ENABLE_RTTI=ON
        -DLLVM_ENABLE_FFI=ON
        -DLLVM_USE_PERF=ON
        -DLLVM_INCLUDE_BENCHMARKS=OFF
        -DLLVM_INCLUDE_EXAMPLES=OFF
        -DLLVM_BUILD_DOCS=OFF
        -DLLVM_INCLUDE_DOCS=OFF
        -DLLVM_ENABLE_OCAMLDOC=OFF
        -DLLVM_ENABLE_SPHINX=OFF
        -DLLVM_ENABLE_BINDINGS=OFF
        -DLLVM_ENABLE_DUMP=ON
        -DLLVM_INCLUDE_TESTS=OFF
        -DLLVM_BUILD_TOOLS=ON
        -DLLVM_OPTIMIZED_TABLEGEN=ON
        -DLLVM_LIT_ARGS="${LITFLAGS} -sv --ignore-fail"
        -Wno-dev
    BUILD_COMMAND ${CMAKE_COMMAND} --build ${LLVM_BINARY_DIR} -j10
    INSTALL_COMMAND ${CMAKE_COMMAND} --build ${LLVM_BINARY_DIR} --target install
    BUILD_ALWAYS OFF
)

# OSMesa External Project
find_program(MESON_EXECUTABLE meson)
if(NOT MESON_EXECUTABLE)
    message(FATAL_ERROR "Meson build system not found. Please install meson")
endif()
ExternalProject_Add(osmesa
    SOURCE_DIR ${MESA_SOURCE_DIR}
    BINARY_DIR ${MESA_BINARY_DIR}
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E env
        "PATH=${LLVM_INSTALL_DIR}/bin:$ENV{PATH}"
        "PKG_CONFIG_PATH=${LLVM_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
        "LLVM_CONFIG=${LLVM_CONFIG}"
        "CMAKE_PREFIX_PATH=${LLVM_INSTALL_DIR}"
        ${MESON_EXECUTABLE} setup ${MESA_BINARY_DIR} ${MESA_SOURCE_DIR}
        -Dprefix=${MESA_INSTALL_DIR}
        -Db_ndebug=true
        -Db_lto=false
        -Db_pie=true
        -Dbuildtype=plain
        --default-library=static
        --wrap-mode=nofallback
        -Dplatforms=
        -Dgallium-drivers=softpipe,llvmpipe
        -Dgallium-vdpau=disabled
        -Dgallium-va=disabled
        -Dgallium-xa=disabled
        -Dgallium-nine=false
        -Dgallium-opencl=disabled
        -Dvulkan-drivers=
        -Dgles1=disabled
        -Dgles2=disabled
        -Dshared-glapi=enabled
        -Dosmesa=true
        -Dopengl=true
        -Dgbm=disabled
        -Dglvnd=disabled
        -Dglx=disabled
        -Degl=disabled
        -Dllvm=enabled
        -Dshared-llvm=disabled
        -Dvalgrind=disabled
        -Dbuild-tests=false
    BUILD_COMMAND
        ${CMAKE_COMMAND} -E env
        "PATH=${LLVM_INSTALL_DIR}/bin:$ENV{PATH}"
        "PKG_CONFIG_PATH=${LLVM_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
        "LLVM_CONFIG=${LLVM_CONFIG}"
        ninja -C ${MESA_BINARY_DIR} -j10
    INSTALL_COMMAND
        ${CMAKE_COMMAND} -E env
        "PATH=${LLVM_INSTALL_DIR}/bin:$ENV{PATH}"
        "PKG_CONFIG_PATH=${LLVM_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
        "LLVM_CONFIG=${LLVM_CONFIG}"
        ninja -C ${MESA_BINARY_DIR} install
    DEPENDS llvm_external
    BUILD_ALWAYS OFF
)

# Post-build verification of OSMesa (removed the problematic verify_osmesa step)

# OSMesa Interface Library
add_library(OSMesa INTERFACE)
add_dependencies(OSMesa osmesa)
target_include_directories(OSMesa INTERFACE ${MESA_INSTALL_DIR}/include)

# Try to get LLVM libraries if llvm-config exists (for subsequent builds)
if(EXISTS ${LLVM_CONFIG})
    execute_process(
        COMMAND ${LLVM_CONFIG} --libfiles
        OUTPUT_VARIABLE LLVM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    string(REPLACE " " ";" LLVM_LIBS "${LLVM_LIBS}")
else()
    # For initial configuration, we'll use a placeholder that gets resolved at build time
    set(LLVM_LIBS "")
    # Create a custom command that generates LLVM library list after LLVM is built
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake
        COMMAND ${CMAKE_COMMAND} -E echo "# Auto-generated LLVM libraries" > ${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake
        COMMAND ${CMAKE_COMMAND} -E echo "set(LLVM_LIBS_GENERATED" >> ${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake
        COMMAND ${LLVM_CONFIG} --libfiles | tr ' ' '\n' | sed 's/^/    /' >> ${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake
        COMMAND ${CMAKE_COMMAND} -E echo ")" >> ${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake
        DEPENDS llvm_external
        COMMENT "Generating LLVM library list"
    )
    add_custom_target(generate_llvm_libs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake)
    add_dependencies(OSMesa generate_llvm_libs)
    
    # Include the generated file if it exists
    if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake)
        include(${CMAKE_CURRENT_BINARY_DIR}/llvm_libs.cmake)
        set(LLVM_LIBS ${LLVM_LIBS_GENERATED})
    endif()
endif()

target_link_libraries(OSMesa INTERFACE
    ${OSMESA_LIB}
    ${LLVM_LIBS}
    pthread
    dl
    m
)

# Create wrapper function for executables with proper RPATH
function(add_relocatable_executable TARGET)
    add_executable(${TARGET} ${ARGN})
    if(BUILD_RELOCATABLE)
        # Set RPATH for the executable
        set_target_properties(${TARGET} PROPERTIES
            BUILD_RPATH "${CMAKE_CURRENT_BINARY_DIR}/llvm-install/lib:${CMAKE_CURRENT_BINARY_DIR}/mesa-install/lib/${LIB_ARCH_DIR}"
            INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:$ORIGIN/../../llvm-install/lib:$ORIGIN/../../mesa-install/lib/${LIB_ARCH_DIR}"
        )
    endif()
endfunction()

# LVGL Library Sources
file(GLOB_RECURSE LVGL_SOURCES 
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/*.cpp)
if(LVGL_INCLUDE_DEMOS)
    file(GLOB_RECURSE LVGL_DEMO_SOURCES 
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/demos/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/demos/*.cpp)
    list(APPEND LVGL_SOURCES ${LVGL_DEMO_SOURCES})
endif()

# SDL sources will be added conditionally below, so remove them first
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*sdl.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*SDL.*")

# SDL Backend Configuration
if(LVGL_USE_SDL)
    find_package(SDL2 CONFIG QUIET)
    if(NOT SDL2_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED sdl2)
        if(NOT TARGET SDL2::SDL2)
            add_library(SDL2::SDL2 INTERFACE IMPORTED)
            set_target_properties(SDL2::SDL2 PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}")
        endif()
    endif()
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS} CACHE STRING "SDL2 include directories")
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES} CACHE STRING "SDL2 libraries")

    list(APPEND LVGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/sdl/lv_sdl_keyboard.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/sdl/lv_sdl_mouse.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/sdl/lv_sdl_mousewheel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/sdl/lv_sdl_window.c)
endif()

# X11 Backend Configuration
if(LVGL_USE_X11)
    list(APPEND LVGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/x11/lv_x11_display.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/x11/lv_x11_input.c)
    find_package(X11 REQUIRED)
endif()

# Wayland Backend Configuration
if(LVGL_USE_WAYLAND)
    file(GLOB WAYLAND_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/wayland/*.c)
    list(APPEND LVGL_SOURCES ${WAYLAND_SOURCES})
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)
endif()

# Linux DRM Backend Configuration
if(LVGL_USE_LINUX_DRM)
    list(APPEND LVGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/display/drm/lv_linux_drm.c)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DRM REQUIRED gbm)
endif()

# Linux Framebuffer Backend Configuration
if(LVGL_USE_LINUX_FBDEV)
    list(APPEND LVGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/display/fb/lv_linux_fbdev.c)
endif()

# NuttX Backend Configuration
if(LVGL_USE_NUTTX)
    file(GLOB NUTTX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/drivers/nuttx/*.c)
    list(APPEND LVGL_SOURCES ${NUTTX_SOURCES})
endif()

# Create LVGL static library
add_library(lvgl STATIC ${LVGL_SOURCES})
set_property(TARGET lvgl PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_include_directories(lvgl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ext
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl
)

# Configure LVGL Target Properties Based on Selected Backend
if(LVGL_USE_SDL)
    target_link_libraries(lvgl PUBLIC SDL2::SDL2)
    target_include_directories(lvgl PUBLIC ${SDL2_INCLUDE_DIRS})
    target_compile_definitions(lvgl PUBLIC LV_USE_SDL=1)
    if(APPLE)
        target_link_options(lvgl PUBLIC "-framework" "SDL2")
    endif()
endif()
if(LVGL_USE_X11)
    target_include_directories(lvgl PUBLIC ${X11_INCLUDE_DIR})
    target_link_libraries(lvgl PUBLIC ${X11_LIBRARIES})
endif()
if(LVGL_USE_WAYLAND)
    target_include_directories(lvgl PUBLIC ${WAYLAND_INCLUDE_DIRS})
    target_link_libraries(lvgl PUBLIC ${WAYLAND_LIBRARIES})
    target_compile_definitions(lvgl PUBLIC LV_USE_WAYLAND=1)
endif()
if(LVGL_USE_LINUX_DRM)
    target_link_libraries(lvgl PUBLIC ${DRM_LIBRARIES})
    target_include_directories(lvgl PUBLIC ${DRM_INCLUDE_DIRS})
    target_compile_definitions(lvgl PUBLIC LV_USE_LINUX_DRM=1)
endif()
if(LVGL_USE_LINUX_FBDEV)
    target_compile_definitions(lvgl PUBLIC LV_USE_LINUX_FBDEV=1)
endif()
if(LVGL_USE_NUTTX)
    target_compile_definitions(lvgl PUBLIC LV_USE_NUTTX=1)
endif()

# Build dx8gl library
add_subdirectory(ext/dx8gl)

# Build src subdirectory
add_subdirectory(src)

# Create convenience scripts
if(BUILD_RELOCATABLE)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_dx8_cube.sh
"#!/bin/bash
SCRIPT_DIR=\"\$(cd \"\$(dirname \"\${BASH_SOURCE[0]}\")\" && pwd)\"
if [[ \"\$OSTYPE\" == \"darwin\"* ]]; then
    export DYLD_LIBRARY_PATH=\"\$SCRIPT_DIR/llvm-install/lib:\$SCRIPT_DIR/mesa-install/lib:\$DYLD_LIBRARY_PATH\"
else
    export LD_LIBRARY_PATH=\"\$SCRIPT_DIR/llvm-install/lib:\$SCRIPT_DIR/mesa-install/lib/${LIB_ARCH_DIR}:\$LD_LIBRARY_PATH\"
fi
exec \"\$SCRIPT_DIR/src/dx8_cube/dx8_cube\" \"\$@\"
")
    file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_dx8_cube.sh
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# CPack configuration for packaging
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME "lvgl_osmesa_llvm")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)

# Install executables, libraries, and run scripts
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/llvm-install/
        DESTINATION llvm-install
        USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mesa-install/
        DESTINATION mesa-install
        USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/
        DESTINATION src
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.exe" PATTERN "*.so" PATTERN "*.dylib")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/run_dx8_cube.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

include(CPack)