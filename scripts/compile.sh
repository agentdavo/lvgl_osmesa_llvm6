#!/bin/bash

# Build script for LVGL OSMesa LLVM Project
# Supports various build options and targets

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
JOBS=8
BUILD_DIR="build"
VERBOSE=false
EGL_ENABLED=false

# Print colored message
print_msg() {
    local color=$1
    shift
    echo -e "${color}$@${NC}"
}

# Print usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS] [TARGET]

Build script for LVGL OSMesa LLVM project

OPTIONS:
    -h, --help          Show this help message
    -c, --clean         Clean build directory
    -C, --clean-all     Clean everything including LLVM/Mesa builds
    -d, --debug         Build in Debug mode (default: Release)
    -j, --jobs N        Number of parallel jobs (default: 8)
    -v, --verbose       Verbose build output
    -r, --reconfigure   Force reconfigure with cmake
    -e, --egl           Enable EGL backend for dx8gl

TARGETS:
    all                 Build everything (default)
    llvm                Build only LLVM
    mesa                Build only Mesa/OSMesa
    lvgl                Build only LVGL library
    examples            Build all examples
    dx8_cube            Build only dx8_cube example

EXAMPLES:
    $0                  # Build everything
    $0 --clean          # Clean and rebuild everything
    $0 --debug dx8_cube # Build dx8_cube in debug mode
    $0 -j 8 llvm        # Build LLVM with 8 parallel jobs
    $0 examples         # Build only the example applications

EOF
}

# Clean build directory
clean_build() {
    if [ -d "$BUILD_DIR" ]; then
        print_msg $YELLOW "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        print_msg $GREEN "Build directory cleaned."
    else
        print_msg $BLUE "Build directory already clean."
    fi
}

# Clean everything including subbuilds
clean_all() {
    print_msg $YELLOW "Cleaning all build artifacts..."
    clean_build
    
    # Clean any other generated files
    find . -name "*.o" -type f -delete 2>/dev/null || true
    find . -name "*.a" -type f -delete 2>/dev/null || true
    
    print_msg $GREEN "All build artifacts cleaned."
}

# Configure cmake
configure_cmake() {
    print_msg $BLUE "Configuring CMake..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    local CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    
    if [ "$VERBOSE" = true ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_VERBOSE_MAKEFILE=ON"
    fi
    
    if [ "$EGL_ENABLED" = true ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DDX8GL_ENABLE_EGL=ON"
    fi
    
    cmake $CMAKE_ARGS ..
    cd ..
    
    print_msg $GREEN "CMake configuration complete."
}

# Build LLVM
build_llvm() {
    print_msg $YELLOW "Building LLVM (this will take 30+ minutes)..."
    cd "$BUILD_DIR"
    make llvm_external -j$JOBS
    cd ..
    print_msg $GREEN "LLVM build complete."
}

# Build Mesa
build_mesa() {
    print_msg $YELLOW "Building Mesa with OSMesa..."
    
    # Check if LLVM is built
    if [ ! -d "$BUILD_DIR/llvm-install" ]; then
        print_msg $RED "Error: LLVM must be built first!"
        print_msg $YELLOW "Run: $0 llvm"
        exit 1
    fi
    
    cd "$BUILD_DIR"
    make osmesa -j$JOBS
    cd ..
    print_msg $GREEN "Mesa build complete."
}

# Build LVGL
build_lvgl() {
    print_msg $YELLOW "Building LVGL library..."
    cd "$BUILD_DIR"
    make lvgl -j$JOBS
    cd ..
    print_msg $GREEN "LVGL build complete."
}

# Build platform library
build_platform() {
    print_msg $YELLOW "Building platform abstraction layer..."
    cd "$BUILD_DIR"
    make lvgl_platform -j$JOBS
    cd ..
    print_msg $GREEN "Platform library build complete."
}

# Build dx8gl
build_dx8gl() {
    print_msg $YELLOW "Building dx8gl library..."
    cd "$BUILD_DIR"
    make dx8gl -j$JOBS
    cd ..
    print_msg $GREEN "dx8gl build complete."
}

# Build specific example
build_example() {
    local target=$1
    print_msg $YELLOW "Building $target..."
    
    # Ensure dependencies are built
    if [ ! -f "$BUILD_DIR/liblvgl.a" ]; then
        build_lvgl
    fi
    
    if [ ! -f "$BUILD_DIR/src/lvgl_platform/liblvgl_platform.a" ]; then
        build_platform
    fi
    
    # For dx8_cube, ensure dx8gl is built
    if [ "$target" = "dx8_cube" ] && [ ! -f "$BUILD_DIR/ext/dx8gl/libdx8gl.a" ]; then
        build_dx8gl
    fi
    
    cd "$BUILD_DIR"
    make $target -j$JOBS
    cd ..
    print_msg $GREEN "$target build complete."
}

# Build all examples
build_examples() {
    print_msg $BLUE "Building all examples..."
    
    # Build dependencies first
    build_lvgl
    build_platform
    build_dx8gl
    
    # Build each example
    build_example "dx8_cube"
    
    print_msg $GREEN "All examples built successfully."
}

# Build everything
build_all() {
    print_msg $BLUE "Building entire project..."
    
    # Check if LLVM is already built to save time
    if [ ! -d "$BUILD_DIR/llvm-install" ]; then
        build_llvm
    else
        print_msg $BLUE "LLVM already built, skipping..."
    fi
    
    # Check if Mesa is already built
    if [ ! -d "$BUILD_DIR/mesa-install" ]; then
        build_mesa
    else
        print_msg $BLUE "Mesa already built, skipping..."
    fi
    
    # Build everything else
    cd "$BUILD_DIR"
    make -j$JOBS
    cd ..
    
    print_msg $GREEN "Build complete!"
    print_msg $BLUE "You can now run the examples:"
    print_msg $NC "  $BUILD_DIR/src/dx8_cube/dx8_cube"
}

# Check build status
check_status() {
    print_msg $BLUE "Build Status:"
    echo -n "  LLVM:     "; [ -d "$BUILD_DIR/llvm-install" ] && print_msg $GREEN "Built" || print_msg $RED "Not built"
    echo -n "  Mesa:     "; [ -d "$BUILD_DIR/mesa-install" ] && print_msg $GREEN "Built" || print_msg $RED "Not built"
    echo -n "  LVGL:     "; [ -f "$BUILD_DIR/liblvgl.a" ] && print_msg $GREEN "Built" || print_msg $RED "Not built"
    echo -n "  dx8gl:    "; [ -f "$BUILD_DIR/ext/dx8gl/libdx8gl.a" ] && print_msg $GREEN "Built" || print_msg $RED "Not built"
    echo -n "  Platform: "; [ -f "$BUILD_DIR/src/lvgl_platform/liblvgl_platform.a" ] && print_msg $GREEN "Built" || print_msg $RED "Not built"
    echo ""
    print_msg $BLUE "Examples:"
    echo -n "  dx8_cube:            "; [ -f "$BUILD_DIR/src/dx8_cube/dx8_cube" ] && print_msg $GREEN "Built" || print_msg $RED "Not built"
}

# Main script logic
CLEAN=false
CLEAN_ALL=false
RECONFIGURE=false
TARGET=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -C|--clean-all)
            CLEAN_ALL=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -r|--reconfigure)
            RECONFIGURE=true
            shift
            ;;
        -e|--egl)
            EGL_ENABLED=true
            shift
            ;;
        *)
            TARGET="$1"
            shift
            ;;
    esac
done

# Print header
print_msg $BLUE "=== LVGL OSMesa LLVM Build Script ==="
echo ""

# Handle clean operations
if [ "$CLEAN_ALL" = true ]; then
    clean_all
    RECONFIGURE=true
elif [ "$CLEAN" = true ]; then
    clean_build
    RECONFIGURE=true
fi

# Configure if needed
if [ ! -d "$BUILD_DIR" ] || [ "$RECONFIGURE" = true ]; then
    configure_cmake
fi

# Handle targets
case "$TARGET" in
    "")
        # No target specified, build all
        build_all
        ;;
    "all")
        build_all
        ;;
    "llvm")
        build_llvm
        ;;
    "mesa")
        build_mesa
        ;;
    "lvgl")
        build_lvgl
        ;;
    "dx8gl")
        build_dx8gl
        ;;
    "platform")
        build_platform
        ;;
    "examples")
        build_examples
        ;;
    "dx8_cube")
        build_example "$TARGET"
        ;;
    "status")
        check_status
        ;;
    *)
        print_msg $RED "Unknown target: $TARGET"
        echo ""
        usage
        exit 1
        ;;
esac

# Show final status
echo ""
check_status