#!/bin/bash
# Build dx8gl samples with Emscripten for browser testing

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building dx8gl with Emscripten ===${NC}"

# Source emsdk environment if available
if [ -f "/home/djs/emsdk/emsdk_env.sh" ]; then
    source "/home/djs/emsdk/emsdk_env.sh"
fi

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo -e "${RED}Error: Emscripten (emcc) not found!${NC}"
    echo "Please install Emscripten:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

# Create build directory
BUILD_DIR="build-emscripten"
echo -e "${YELLOW}Creating build directory: ${BUILD_DIR}${NC}"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Configure with CMake for Emscripten
echo -e "${YELLOW}Configuring with CMake...${NC}"
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_DX8GL_SAMPLES=ON \
    -DCMAKE_VERBOSE_MAKEFILE=ON

# Build
echo -e "${YELLOW}Building...${NC}"
emmake make -j$(nproc)

echo -e "${GREEN}=== Build Complete ===${NC}"
echo -e "${YELLOW}HTML files generated in: ${BUILD_DIR}/samples/${NC}"

# List generated HTML files
echo -e "\n${GREEN}Generated samples:${NC}"
find samples -name "*.html" -type f | while read file; do
    echo "  - $file"
done

echo -e "\n${YELLOW}To test the samples:${NC}"
echo "  1. Run the test server: ../serve_emscripten.py"
echo "  2. Open http://localhost:8000 in your browser"
echo "  3. Navigate to the samples/ directory"