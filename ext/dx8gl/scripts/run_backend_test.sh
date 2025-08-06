#!/bin/bash

# Script to run cross-backend rendering tests and report differences

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "==================================="
echo "Cross-Backend Rendering Test Suite"
echo "==================================="
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found at $BUILD_DIR${NC}"
    echo "Please build the project first with:"
    echo "  cd $PROJECT_ROOT"
    echo "  cmake -S . -B build"
    echo "  cmake --build build"
    exit 1
fi

# Check which backends are available
BACKENDS=""
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    if grep -q "DX8GL_ENABLE_EGL:BOOL=ON" "$BUILD_DIR/CMakeCache.txt"; then
        BACKENDS="$BACKENDS EGL"
    fi
    if grep -q "DX8GL_ENABLE_WEBGPU:BOOL=ON" "$BUILD_DIR/CMakeCache.txt"; then
        BACKENDS="$BACKENDS WebGPU"
    fi
fi

echo "Available backends: OSMesa${BACKENDS}"
echo ""

# Run the cross-backend test
TEST_EXECUTABLE="$BUILD_DIR/test/test_cross_backend_rendering"

if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${RED}Error: Test executable not found at $TEST_EXECUTABLE${NC}"
    echo "Please build the tests with:"
    echo "  cd $PROJECT_ROOT"
    echo "  cmake --build build --target test_cross_backend_rendering"
    exit 1
fi

# Create output directory for test results
OUTPUT_DIR="$PROJECT_ROOT/test_output"
mkdir -p "$OUTPUT_DIR"

echo "Running cross-backend tests..."
echo "Output will be saved to: $OUTPUT_DIR"
echo ""

# Set library paths
export LD_LIBRARY_PATH="$PROJECT_ROOT/build/llvm-install/lib:$PROJECT_ROOT/build/mesa-install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

# Run the test with verbose output
cd "$OUTPUT_DIR"
if "$TEST_EXECUTABLE" --gtest_color=yes; then
    echo ""
    echo -e "${GREEN}✓ All backend tests passed!${NC}"
    RESULT=0
else
    echo ""
    echo -e "${RED}✗ Some backend tests failed!${NC}"
    RESULT=1
fi

# Check for output images
echo ""
echo "Generated test images:"
for ppm in *.ppm; do
    if [ -f "$ppm" ]; then
        echo "  - $ppm"
    fi
done

# Optional: Convert PPM to PNG if ImageMagick is available
if command -v convert &> /dev/null; then
    echo ""
    echo "Converting PPM images to PNG..."
    for ppm in *.ppm; do
        if [ -f "$ppm" ]; then
            png="${ppm%.ppm}.png"
            convert "$ppm" "$png"
            echo "  - Created $png"
        fi
    done
fi

# Generate comparison report
echo ""
echo "==================================="
echo "Backend Comparison Report"
echo "==================================="

# Check for significant differences
if [ -f "triangle_OSMesa.ppm" ]; then
    echo ""
    echo "Triangle rendering test:"
    echo "  Reference: OSMesa"
    
    for backend in $BACKENDS; do
        backend_file="triangle_${backend// /}.ppm"
        if [ -f "$backend_file" ]; then
            echo "  ✓ $backend rendered successfully"
        else
            echo "  ✗ $backend failed to render"
        fi
    done
fi

if [ -f "textured_OSMesa.ppm" ]; then
    echo ""
    echo "Textured quad test:"
    echo "  Reference: OSMesa"
    
    for backend in $BACKENDS; do
        backend_file="textured_${backend// /}.ppm"
        if [ -f "$backend_file" ]; then
            echo "  ✓ $backend rendered successfully"
        else
            echo "  ✗ $backend failed to render"
        fi
    done
fi

if [ -f "alpha_OSMesa.ppm" ]; then
    echo ""
    echo "Alpha blending test:"
    echo "  Reference: OSMesa"
    
    for backend in $BACKENDS; do
        backend_file="alpha_${backend// /}.ppm"
        if [ -f "$backend_file" ]; then
            echo "  ✓ $backend rendered successfully"
        else
            echo "  ✗ $backend failed to render"
        fi
    done
fi

echo ""
echo "==================================="
if [ $RESULT -eq 0 ]; then
    echo -e "${GREEN}Test suite completed successfully!${NC}"
else
    echo -e "${YELLOW}Test suite completed with some failures.${NC}"
    echo "Check the output above for details."
fi
echo "==================================="

exit $RESULT