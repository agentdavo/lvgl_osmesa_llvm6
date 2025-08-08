#!/bin/bash
# Script to run individual Google Test executables with full logging
# Usage: ./run_gtest_with_logs.sh <test_executable> [gtest_options]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
LOG_DIR="$PROJECT_ROOT/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Check arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_executable> [gtest_options]"
    echo ""
    echo "Examples:"
    echo "  $0 test_d3dx_math"
    echo "  $0 test_d3dx_color --gtest_filter='*ColorLerp*'"
    echo "  $0 test_backend_selection --gtest_repeat=3"
    echo ""
    echo "Common gtest options:"
    echo "  --gtest_list_tests        List all tests without running"
    echo "  --gtest_filter=PATTERN    Run only tests matching pattern"
    echo "  --gtest_repeat=N           Run tests N times"
    echo "  --gtest_shuffle            Randomize test order"
    echo "  --gtest_output=xml:file   Generate XML test report"
    echo "  --gtest_break_on_failure  Invoke debugger on failure"
    exit 1
fi

TEST_NAME="$1"
shift
GTEST_OPTIONS="$@"

# Find the test executable
if [ -f "$BUILD_DIR/Release/$TEST_NAME" ]; then
    TEST_PATH="$BUILD_DIR/Release/$TEST_NAME"
elif [ -f "$BUILD_DIR/ext/dx8gl/test/$TEST_NAME" ]; then
    TEST_PATH="$BUILD_DIR/ext/dx8gl/test/$TEST_NAME"
elif [ -f "$BUILD_DIR/$TEST_NAME" ]; then
    TEST_PATH="$BUILD_DIR/$TEST_NAME"
else
    echo -e "${RED}Error: Test executable '$TEST_NAME' not found${NC}"
    echo "Searched in:"
    echo "  - $BUILD_DIR/Release/"
    echo "  - $BUILD_DIR/ext/dx8gl/test/"
    echo "  - $BUILD_DIR/"
    exit 1
fi

# Create log directory
TEST_LOG_DIR="$LOG_DIR/gtest_runs/${TEST_NAME}_${TIMESTAMP}"
mkdir -p "$TEST_LOG_DIR"

# Log files
CONSOLE_LOG="$TEST_LOG_DIR/console_output.log"
GTEST_LOG="$TEST_LOG_DIR/gtest_output.log"
XML_REPORT="$TEST_LOG_DIR/test_results.xml"

# Set library paths
export LD_LIBRARY_PATH="$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:$BUILD_DIR/ext/dx8gl:$LD_LIBRARY_PATH"

# Set backend to OSMesa by default
export DX8GL_BACKEND="${DX8GL_BACKEND:-osmesa}"

echo -e "${BLUE}=== Google Test Runner with Logging ===${NC}"
echo "Test: $TEST_NAME"
echo "Path: $TEST_PATH"
echo "Backend: $DX8GL_BACKEND"
echo "Log directory: $TEST_LOG_DIR"
echo "Timestamp: $TIMESTAMP"
echo ""

# Write header to console log
{
    echo "================================================================================="
    echo "Google Test Execution Log"
    echo "================================================================================="
    echo "Test Name: $TEST_NAME"
    echo "Test Path: $TEST_PATH"
    echo "Backend: $DX8GL_BACKEND"
    echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "GTest Options: $GTEST_OPTIONS"
    echo "================================================================================="
    echo ""
} > "$CONSOLE_LOG"

# Run the test with Google Test options
echo -e "${CYAN}Running test...${NC}"
START_TIME=$(date +%s)

# Run test and capture output
if "$TEST_PATH" $GTEST_OPTIONS --gtest_output=xml:"$XML_REPORT" 2>&1 | tee -a "$CONSOLE_LOG" | tee "$GTEST_LOG"; then
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    TEST_RESULT="PASSED"
    RESULT_COLOR="${GREEN}"
else
    EXIT_CODE=$?
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    TEST_RESULT="FAILED"
    RESULT_COLOR="${RED}"
fi

# Append summary to console log
{
    echo ""
    echo "================================================================================="
    echo "Test Execution Summary"
    echo "================================================================================="
    echo "Result: $TEST_RESULT"
    echo "Duration: ${DURATION} seconds"
    echo "Exit Code: ${EXIT_CODE:-0}"
    echo "================================================================================="
} >> "$CONSOLE_LOG"

# Parse test results from output
if [ -f "$GTEST_LOG" ]; then
    TOTAL_TESTS=$(grep -E "^\[==========\] Running" "$GTEST_LOG" | sed -E 's/.*Running ([0-9]+) test.*/\1/' | tail -1)
    PASSED_TESTS=$(grep -E "^\[  PASSED  \]" "$GTEST_LOG" | sed -E 's/.*\[  PASSED  \] ([0-9]+) test.*/\1/' | tail -1)
    FAILED_TESTS=$(grep -E "^\[  FAILED  \]" "$GTEST_LOG" | sed -E 's/.*\[  FAILED  \] ([0-9]+) test.*/\1/' | tail -1)
    
    # Show summary
    echo ""
    echo -e "${BLUE}=== Test Results ===${NC}"
    echo -e "Result: ${RESULT_COLOR}${TEST_RESULT}${NC}"
    echo "Duration: ${DURATION} seconds"
    if [ -n "$TOTAL_TESTS" ]; then
        echo "Total tests: $TOTAL_TESTS"
        [ -n "$PASSED_TESTS" ] && echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
        [ -n "$FAILED_TESTS" ] && echo -e "Failed: ${RED}$FAILED_TESTS${NC}"
    fi
fi

# Create summary file
SUMMARY_FILE="$TEST_LOG_DIR/summary.txt"
{
    echo "Test: $TEST_NAME"
    echo "Result: $TEST_RESULT"
    echo "Duration: ${DURATION}s"
    echo "Timestamp: $TIMESTAMP"
    echo ""
    echo "Log files:"
    echo "  Console output: $CONSOLE_LOG"
    echo "  GTest output: $GTEST_LOG"
    echo "  XML report: $XML_REPORT"
} > "$SUMMARY_FILE"

echo ""
echo -e "${BLUE}=== Log Files ===${NC}"
echo "Console output: $CONSOLE_LOG"
echo "GTest output: $GTEST_LOG"
echo "XML report: $XML_REPORT"
echo "Summary: $SUMMARY_FILE"

# Create symlink to latest test run
ln -sfn "$TEST_LOG_DIR" "$LOG_DIR/gtest_runs/latest_${TEST_NAME}"
echo ""
echo "Latest run linked at: $LOG_DIR/gtest_runs/latest_${TEST_NAME}"

# Exit with same code as test
exit ${EXIT_CODE:-0}