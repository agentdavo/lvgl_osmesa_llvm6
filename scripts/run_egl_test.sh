#!/bin/bash

# Run script for EGL backend test

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Check if binary exists
BINARY="$PROJECT_ROOT/build/src/egl_backend_test/egl_backend_test"
if [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: egl_backend_test not found at $BINARY${NC}"
    echo -e "${YELLOW}Build with: ./scripts/compile.sh -e egl_backend_test${NC}"
    exit 1
fi

# Set library paths for custom LLVM and Mesa
export LD_LIBRARY_PATH="$PROJECT_ROOT/build/llvm-install/lib:$PROJECT_ROOT/build/mesa-install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

# Enable EGL backend via environment
export DX8GL_BACKEND=egl

echo -e "${GREEN}Running EGL backend test...${NC}"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "DX8GL_BACKEND=$DX8GL_BACKEND"
echo ""

# Run the test
"$BINARY" "$@"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}EGL backend test completed successfully${NC}"
    
    # Check if output file was created
    if [ -f "egl_test_output.ppm" ]; then
        echo -e "${GREEN}Output saved to egl_test_output.ppm${NC}"
    fi
else
    echo -e "${RED}EGL backend test failed with exit code: $EXIT_CODE${NC}"
fi

exit $EXIT_CODE