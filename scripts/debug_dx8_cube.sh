#!/bin/bash
# Script to debug dx8_cube with gdb

echo "Debugging dx8_cube with gdb..."

# Set library path and run with gdb
export LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu

# Run gdb with batch commands to get backtrace
gdb -batch \
    -ex "set pagination off" \
    -ex "set confirm off" \
    -ex "run" \
    -ex "bt" \
    -ex "info registers" \
    -ex "x/10i \$pc-20" \
    -ex "info threads" \
    -ex "thread apply all bt" \
    build/src/dx8_cube/dx8_cube 2>&1 | tee gdb_output.txt

echo ""
echo "GDB output saved to gdb_output.txt"

# Show summary of crash
echo ""
echo "=== Crash Summary ==="
grep -A5 "Program received signal" gdb_output.txt || echo "No crash signal found"
echo ""
echo "=== Top of backtrace ==="
grep -A10 "^#0" gdb_output.txt | head -15