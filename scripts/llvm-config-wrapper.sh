#!/bin/bash
# llvm-config wrapper that returns relative paths

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Path to the actual llvm-config
REAL_LLVM_CONFIG="$BUILD_DIR/llvm-install/bin/llvm-config"

if [ ! -x "$REAL_LLVM_CONFIG" ]; then
    echo "Error: llvm-config not found at $REAL_LLVM_CONFIG" >&2
    exit 1
fi

# Function to convert absolute paths to relative
make_relative() {
    local output="$1"
    # Replace absolute paths with relative ones
    echo "$output" | sed "s|$BUILD_DIR|\$BUILD_DIR|g"
}

# Check if we need to process the output
case "$1" in
    --libfiles|--libs|--ldflags|--system-libs|--libdir|--includedir|--prefix|--obj-root|--src-root)
        # These commands return paths that need to be made relative
        output=$("$REAL_LLVM_CONFIG" "$@")
        make_relative "$output"
        ;;
    *)
        # For other commands, just pass through
        "$REAL_LLVM_CONFIG" "$@"
        ;;
esac