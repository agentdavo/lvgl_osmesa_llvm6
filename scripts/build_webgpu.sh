#!/bin/bash

# Build script for WebGPU targets with Emscripten
# Supports various WebGPU configurations and threading modes

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build-webgpu"
THREADING="none"
VERBOSE=false
CLEAN=false
TARGET="all"

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

Build script for WebGPU targets with Emscripten

OPTIONS:
    -h, --help              Show this help message
    -c, --clean             Clean build directory before building
    -d, --debug             Build in Debug mode (default: Release)
    -v, --verbose           Verbose build output
    -t, --threading MODE    Threading mode: none, pthreads, wasm-workers (default: none)
    -o, --output DIR        Output directory (default: build-webgpu)

TARGETS:
    all                     Build all WebGPU samples (default)
    webgpu_complex_scene    Build complex scene demo
    webgpu_test             Build WebGPU backend test
    harness                 Copy HTML/JS harness files

THREADING MODES:
    none                    Single-threaded, main thread rendering
    pthreads                Use Emscripten pthreads (requires COOP/COEP headers)
    wasm-workers            Use Wasm Workers for parallelism

EXAMPLES:
    $0                              # Build all WebGPU targets
    $0 -t pthreads webgpu_complex_scene  # Build with pthreads
    $0 -c -d                        # Clean build in debug mode
    $0 harness                      # Just copy HTML/JS files

PREREQUISITES:
    - Emscripten SDK (emsdk) must be activated
    - Source emsdk_env.sh before running this script

EOF
}

# Check Emscripten
check_emscripten() {
    if [ -z "$EMSDK" ]; then
        print_msg $RED "Error: Emscripten SDK not found!"
        print_msg $YELLOW "Please activate emsdk first:"
        print_msg $NC "  source /path/to/emsdk/emsdk_env.sh"
        exit 1
    fi
    
    print_msg $GREEN "Found Emscripten SDK: $EMSDK"
    emcc --version | head -1
}

# Clean build directory
clean_build() {
    if [ -d "$BUILD_DIR" ]; then
        print_msg $YELLOW "Cleaning $BUILD_DIR..."
        rm -rf "$BUILD_DIR"
    fi
}

# Configure CMake for Emscripten
configure_cmake() {
    print_msg $BLUE "Configuring CMake for WebGPU build..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Base CMake arguments
    local CMAKE_ARGS=(
        -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DDX8GL_ENABLE_WEBGPU=ON
        -DDX8GL_WEBGPU_THREADING="$THREADING"
        -DBUILD_DX8GL_SAMPLES=ON
        -DBUILD_DX8GL_TESTS=OFF
    )
    
    if [ "$VERBOSE" = true ]; then
        CMAKE_ARGS+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
    fi
    
    print_msg $CYAN "CMake arguments: ${CMAKE_ARGS[@]}"
    
    cmake "${CMAKE_ARGS[@]}" ../..
    cd ..
    
    print_msg $GREEN "CMake configuration complete."
}

# Build specific target
build_target() {
    local target=$1
    print_msg $YELLOW "Building $target..."
    
    cd "$BUILD_DIR"
    
    if [ "$target" = "all" ]; then
        make -j$(nproc)
    else
        make $target -j$(nproc)
    fi
    
    cd ..
    
    print_msg $GREEN "$target build complete."
}

# Copy harness files
copy_harness() {
    print_msg $BLUE "Copying WebGPU harness files..."
    
    mkdir -p "$BUILD_DIR"
    
    # Copy HTML/JS harness files
    cp ext/dx8gl/samples/webgpu_harness.html "$BUILD_DIR/" 2>/dev/null || true
    cp ext/dx8gl/samples/webgpu_worker.js "$BUILD_DIR/" 2>/dev/null || true
    
    # Create index.html with links to all demos
    cat > "$BUILD_DIR/index.html" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>dx8gl WebGPU Demos</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        h1 { text-align: center; }
        .demo-list {
            list-style: none;
            padding: 0;
        }
        .demo-list li {
            margin: 20px 0;
        }
        .demo-list a {
            display: block;
            padding: 20px;
            background: rgba(255,255,255,0.2);
            border-radius: 10px;
            color: white;
            text-decoration: none;
            transition: background 0.3s;
        }
        .demo-list a:hover {
            background: rgba(255,255,255,0.3);
        }
        .info {
            background: rgba(0,0,0,0.3);
            padding: 20px;
            border-radius: 10px;
            margin-top: 30px;
        }
        code {
            background: rgba(0,0,0,0.5);
            padding: 2px 5px;
            border-radius: 3px;
        }
    </style>
</head>
<body>
    <h1>🎮 dx8gl WebGPU Demos</h1>
    
    <ul class="demo-list">
        <li>
            <a href="webgpu_harness.html">
                <h3>WebGPU Harness</h3>
                <p>Interactive harness for testing WebGPU backend with different threading modes</p>
            </a>
        </li>
        <li>
            <a href="dx8gl_webgpu_complex_scene.html">
                <h3>Complex Scene Demo</h3>
                <p>Multiple rotating objects with dynamic vertex updates</p>
            </a>
        </li>
    </ul>
    
    <div class="info">
        <h3>Requirements:</h3>
        <ul>
            <li>Chrome 113+ or Edge 113+ (WebGPU support)</li>
            <li>For pthreads mode: COOP/COEP headers required</li>
        </ul>
        
        <h3>Local Server:</h3>
        <p>Run a local server with appropriate headers:</p>
        <code>python3 serve_webgpu.py</code>
    </div>
</body>
</html>
EOF
    
    # Create Python server script with COOP/COEP headers
    cat > "$BUILD_DIR/serve_webgpu.py" << 'EOF'
#!/usr/bin/env python3
"""
Simple HTTP server with COOP/COEP headers for WebGPU demos
"""
import http.server
import socketserver
import os

class WebGPUHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add COOP/COEP headers for SharedArrayBuffer and pthreads
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        # Add cache control
        self.send_header('Cache-Control', 'no-cache')
        super().end_headers()

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

os.chdir(DIRECTORY)

with socketserver.TCPServer(("", PORT), WebGPUHTTPRequestHandler) as httpd:
    print(f"Server running at http://localhost:{PORT}/")
    print("COOP/COEP headers enabled for WebGPU and pthreads support")
    print("Press Ctrl+C to stop")
    httpd.serve_forever()
EOF
    
    chmod +x "$BUILD_DIR/serve_webgpu.py"
    
    print_msg $GREEN "Harness files copied to $BUILD_DIR/"
}

# Generate build report
generate_report() {
    print_msg $BLUE "Build Summary:"
    echo "================="
    echo "Build Type:      $BUILD_TYPE"
    echo "Threading Mode:  $THREADING"
    echo "Output Dir:      $BUILD_DIR"
    echo ""
    
    if [ -d "$BUILD_DIR" ]; then
        print_msg $CYAN "Generated files:"
        find "$BUILD_DIR" -name "*.html" -o -name "*.js" -o -name "*.wasm" | head -20
        
        echo ""
        print_msg $GREEN "To run the demos:"
        print_msg $NC "  cd $BUILD_DIR"
        print_msg $NC "  python3 serve_webgpu.py"
        print_msg $NC "  Open http://localhost:8080/ in Chrome/Edge"
    fi
}

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
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -t|--threading)
            THREADING="$2"
            shift 2
            ;;
        -o|--output)
            BUILD_DIR="$2"
            shift 2
            ;;
        *)
            TARGET="$1"
            shift
            ;;
    esac
done

# Main execution
print_msg $BLUE "=== dx8gl WebGPU Build Script ==="
echo ""

# Check Emscripten
check_emscripten

# Validate threading mode
case $THREADING in
    none|pthreads|wasm-workers)
        print_msg $CYAN "Threading mode: $THREADING"
        ;;
    *)
        print_msg $RED "Invalid threading mode: $THREADING"
        print_msg $YELLOW "Valid modes: none, pthreads, wasm-workers"
        exit 1
        ;;
esac

# Clean if requested
if [ "$CLEAN" = true ]; then
    clean_build
fi

# Configure and build
if [ "$TARGET" = "harness" ]; then
    copy_harness
else
    # Ensure CMake is configured
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$CLEAN" = true ]; then
        configure_cmake
    fi
    
    # Build target
    build_target "$TARGET"
    
    # Copy harness files after build
    copy_harness
fi

# Generate report
generate_report

print_msg $GREEN "Build complete!"