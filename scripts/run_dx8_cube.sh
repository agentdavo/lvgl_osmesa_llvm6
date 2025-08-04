#!/bin/bash
# Script to run dx8_cube with proper environment and save logs
# Usage: ./run_dx8_cube.sh [--backend=osmesa|egl]

# Source common backend script
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common_backend.sh"

# Parse backend option
parse_backend_option "$@"

# Create logs directory if it doesn't exist
mkdir -p logs

# Get timestamp for log file
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOGFILE="logs/dx8_cube_run_${TIMESTAMP}.txt"

echo "Running dx8_cube..."
echo "Log file: $LOGFILE"

# Set library path and run dx8_cube, saving all output
export LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu

# Change to the executable directory so it can find resources
cd build/src/dx8_cube

# Run with timeout and capture all output (use absolute path for log)
timeout 10 ./dx8_cube 2>&1 | tee "../../../$LOGFILE"

# Get exit code
EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "Exit code: $EXIT_CODE"
echo "Log saved to: $LOGFILE"

# Print summary
if [ $EXIT_CODE -eq 124 ]; then
    echo "Program timed out after 10 seconds"
elif [ $EXIT_CODE -eq 0 ]; then
    echo "Program exited normally"
else
    echo "Program crashed with exit code $EXIT_CODE"
fi

# Show last 20 lines of log
echo ""
echo "=== Last 20 lines of output ==="
tail -20 "$LOGFILE"