#!/bin/bash
# Script to run shader-related tests with proper OSMesa library path

# Change to the project root directory
cd "$(dirname "$0")/.."

# Set library path to use local OSMesa 25.0.7
export LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu

echo "Running shader tests with local OSMesa..."
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo ""

# Check OSMesa version
echo "Checking OSMesa version..."
if [ -f build/Release/test_shader_translator ]; then
    ldd build/Release/test_shader_translator | grep -i mesa || echo "Note: Unable to check OSMesa linkage"
fi
echo ""

# Run tests that were successfully created
echo "=== Running Shader Translator Test ==="
if [ -f build/Release/test_shader_translator ]; then
    build/Release/test_shader_translator
    echo ""
else
    echo "test_shader_translator not found"
fi

# Note: The advanced shader tests may not have been built due to missing dx8gl target linkage
echo "=== Running Core API Test ==="
if [ -f build/Release/test_dx8gl_core_api ]; then
    echo "Running dx8gl Core API Test..."
    build/Release/test_dx8gl_core_api
    echo ""
else
    echo "test_dx8gl_core_api not found"
fi

echo "=== Running Other Available Tests ==="
if [ -f build/Release/test_lod_dirty_logic ]; then
    echo "Running LOD Dirty Logic Test..."
    build/Release/test_lod_dirty_logic
    echo ""
fi

if [ -f build/Release/test_multi_texcoords ]; then
    echo "Running Multi Texture Coordinates Test..."
    build/Release/test_multi_texcoords
    echo ""
fi

echo "All available tests completed."