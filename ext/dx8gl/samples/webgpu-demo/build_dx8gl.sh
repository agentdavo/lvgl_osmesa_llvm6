#!/bin/bash

echo "Building dx8gl WebGPU Demo..."

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten not found. Please install and activate emsdk."
    exit 1
fi

# Paths
DX8GL_SRC="../../src"

# Build flags for dx8gl with WebGPU
CFLAGS="-O2 -std=c++17"
CFLAGS="$CFLAGS -I$DX8GL_SRC"
CFLAGS="$CFLAGS -I../../lib/lib_webgpu"
CFLAGS="$CFLAGS -DDX8GL_ENABLE_WEBGPU=1"
CFLAGS="$CFLAGS -DDX8GL_HAS_WEBGPU=1"
CFLAGS="$CFLAGS -D__EMSCRIPTEN__"

LDFLAGS="-s USE_WEBGPU=1"
LDFLAGS="$LDFLAGS -s WASM=1"
LDFLAGS="$LDFLAGS -s ALLOW_MEMORY_GROWTH=1"
LDFLAGS="$LDFLAGS -s NO_EXIT_RUNTIME=1"
LDFLAGS="$LDFLAGS -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
LDFLAGS="$LDFLAGS -s EXPORTED_FUNCTIONS=['_main']"
LDFLAGS="$LDFLAGS -s ASYNCIFY"
LDFLAGS="$LDFLAGS -s FULL_ES3=1"

# Collect essential dx8gl source files for WebGPU backend
SOURCES="dx8_webgpu_demo.cpp"
SOURCES="$SOURCES $DX8GL_SRC/dx8gl.cpp"
SOURCES="$SOURCES $DX8GL_SRC/d3d8_interface.cpp"
SOURCES="$SOURCES $DX8GL_SRC/d3d8_device.cpp"
SOURCES="$SOURCES $DX8GL_SRC/d3d8_vertexbuffer.cpp"
SOURCES="$SOURCES $DX8GL_SRC/d3d8_indexbuffer.cpp"
SOURCES="$SOURCES $DX8GL_SRC/d3d8_texture.cpp"
SOURCES="$SOURCES $DX8GL_SRC/d3d8_surface.cpp"
SOURCES="$SOURCES $DX8GL_SRC/state_manager.cpp"
SOURCES="$SOURCES $DX8GL_SRC/webgpu_backend.cpp"
SOURCES="$SOURCES $DX8GL_SRC/offscreen_framebuffer.cpp"
SOURCES="$SOURCES $DX8GL_SRC/d3dx_math.cpp"

# Also need the lib_webgpu implementation
if [ -f "../../lib/lib_webgpu/lib_webgpu.cpp" ]; then
    SOURCES="$SOURCES ../../lib/lib_webgpu/lib_webgpu.cpp"
fi

# Compile the demo with dx8gl library
echo "Compiling dx8gl with WebGPU backend..."
emcc $SOURCES \
    $CFLAGS \
    $LDFLAGS \
    -o dx8_webgpu_demo.js

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo ""
    echo "To run the demo:"
    echo "1. Start the server: python3 serve_dx8gl.py"
    echo "2. Open: http://localhost:8090/dx8gl_demo.html"
else
    echo "Build failed!"
    exit 1
fi