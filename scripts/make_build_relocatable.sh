#!/bin/bash
# Script to make the build relocatable after compilation

set -e

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Making build relocatable...${NC}"

# Get the absolute path to the project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Create a relocatable environment script
cat > "$BUILD_DIR/setup_env.sh" << 'EOF'
#!/bin/bash
# Setup environment for relocatable build

# Get the directory where this script is located (the build directory)
BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$BUILD_DIR/.." && pwd)"

# Set up paths
export LD_LIBRARY_PATH="$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"
export PATH="$BUILD_DIR/llvm-install/bin:$PATH"
export PKG_CONFIG_PATH="$BUILD_DIR/llvm-install/lib/pkgconfig:$BUILD_DIR/mesa-install/lib/pkgconfig:$PKG_CONFIG_PATH"

# Set LLVM_CONFIG to use our installed version
export LLVM_CONFIG="$BUILD_DIR/llvm-install/bin/llvm-config"

echo "Environment configured for relocatable build at: $BUILD_DIR"
echo "You can now run examples with:"
echo "  $BUILD_DIR/src/dx8_cube/dx8_cube"
EOF

chmod +x "$BUILD_DIR/setup_env.sh"

# Create wrapper scripts for examples
for example in dx8_cube osmesa_test lvgl_hello lvgl_osmesa; do
    if [ -f "$BUILD_DIR/src/$example/$example" ]; then
        cat > "$BUILD_DIR/run_${example}.sh" << EOF
#!/bin/bash
# Run $example with correct library paths

SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="\$SCRIPT_DIR/llvm-install/lib:\$SCRIPT_DIR/mesa-install/lib/x86_64-linux-gnu:\$LD_LIBRARY_PATH"
exec "\$SCRIPT_DIR/src/$example/$example" "\$@"
EOF
        chmod +x "$BUILD_DIR/run_${example}.sh"
    fi
done

# Create a README for the relocatable build
cat > "$BUILD_DIR/README_RELOCATABLE.txt" << 'EOF'
RELOCATABLE BUILD
=================

This build directory can be moved or copied to any location.

To use the build after moving/copying:

1. Source the environment setup:
   source ./setup_env.sh

2. Run examples using the wrapper scripts:
   ./run_dx8_cube.sh
   ./run_osmesa_test.sh
   ./run_lvgl_hello.sh
   ./run_lvgl_osmesa.sh

Or run directly after sourcing setup_env.sh:
   ./src/dx8_cube/dx8_cube

The build includes:
- LLVM (in llvm-install/)
- Mesa with OSMesa (in mesa-install/)
- All compiled examples (in src/)

Note: The source code is not required to run the compiled binaries.
Only this build directory is needed.
EOF

echo -e "${GREEN}Build is now relocatable!${NC}"
echo ""
echo "The build directory can be copied/moved anywhere."
echo "After moving, run: source build/setup_env.sh"
echo "Then run examples with: build/run_dx8_cube.sh"