cmake_minimum_required(VERSION 3.20)
project(lvgl_osmesa_llvm C CXX)

# Enforce C++ standard for compatibility
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LLVM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/llvm-project/llvm)
set(LLVM_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-build)
set(LLVM_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-install)
set(LLVM_CONFIG ${LLVM_INSTALL_DIR}/bin/llvm-config)

set(MESA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/mesa)
set(MESA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mesa-build)
set(MESA_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/mesa-install)
set(OSMESA_LIB "${MESA_INSTALL_DIR}/lib/x86_64-linux-gnu/libOSMesa.so")

include(ExternalProject)

# LVGL Display Backend Options
option(LVGL_USE_X11 "Enable LVGL X11 backend" ON)
option(LVGL_USE_SDL "Enable LVGL SDL backend" ON)
option(LVGL_USE_WAYLAND "Enable LVGL Wayland backend" OFF)
option(LVGL_USE_LINUX_DRM "Enable LVGL DRM backend" OFF)
option(LVGL_USE_LINUX_FBDEV "Enable LVGL FBDev backend" OFF)
option(LVGL_USE_NUTTX "Enable LVGL NuttX backend" OFF)

# Minimal LLVM build configuration for Mesa
# Additional optimizations to reduce build time and size

# LLVM External Project
ExternalProject_Add(llvm_external
    SOURCE_DIR ${LLVM_SOURCE_DIR}
    BINARY_DIR ${LLVM_BINARY_DIR}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=MinSizeRel
		-DCMAKE_INSTALL_PREFIX=${LLVM_INSTALL_DIR}
        -DCMAKE_SKIP_INSTALL_RPATH=TRUE
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
        -D prefix=${MESA_INSTALL_DIR}
		-D b_ndebug=true
        -D b_lto=false
        -D b_pie=true
        -D buildtype=plain
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

# OSMesa Interface Library
add_library(OSMesa INTERFACE)
add_dependencies(OSMesa osmesa)
target_include_directories(OSMesa INTERFACE ${MESA_INSTALL_DIR}/include)

execute_process(
    COMMAND ${LLVM_CONFIG} --libfiles
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE " " ";" LLVM_LIBS "${LLVM_LIBS}")

target_link_libraries(OSMesa INTERFACE
    ${OSMESA_LIB}
    ${LLVM_LIBS}
    pthread
    dl
    m
)

# ================================================================================
# LVGL Library Sources
# ================================================================================
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/demos/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/lvgl/demos/*.cpp)

# SDL sources will be added conditionally below, so remove them first
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*sdl.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*SDL.*")

# SDL Backend Configuration
if(LVGL_USE_SDL)
    # Find SDL2 first before adding sources
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
    # Export SDL2 variables so other targets can use them
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS} CACHE STRING "SDL2 include directories")
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES} CACHE STRING "SDL2 libraries")
    
    # Now add SDL sources
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
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/drivers/wayland/*.c)
    list(APPEND LVGL_SOURCES ${WAYLAND_SOURCES})
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)
endif()

# Linux DRM Backend Configuration
if(LVGL_USE_LINUX_DRM)
    list(APPEND LVGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/drivers/display/drm/lv_linux_drm.c)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DRM REQUIRED gbm)
endif()

# Linux Framebuffer Backend Configuration
if(LVGL_USE_LINUX_FBDEV)
    list(APPEND LVGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/drivers/display/fb/lv_linux_fbdev.c)
endif()

# NuttX Backend Configuration
if(LVGL_USE_NUTTX)
    file(GLOB NUTTX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/drivers/nuttx/*.c)
    list(APPEND LVGL_SOURCES ${NUTTX_SOURCES})
endif()

# Create LVGL static library
add_library(lvgl STATIC ${LVGL_SOURCES})

# Enable position-independent code for static library (needed for linking into shared libraries)
set_property(TARGET lvgl PROPERTY POSITION_INDEPENDENT_CODE ON)

# Define LV_CONF_INCLUDE_SIMPLE to use lv_conf.h from the project root
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
        # When SDL2 is installed as a framework on macOS, CMake's package
        # configuration may not inject the required "-framework SDL2" linker
        # option.  Add it explicitly so lvgl links correctly.
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

# Build src subdirectory which contains all our code
add_subdirectory(src)