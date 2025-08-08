#!/bin/bash
# Automated test runner for dx8gl library
# This script builds and runs all dx8gl tests with proper library paths

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
LOG_DIR="$PROJECT_ROOT/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_LOG_DIR="$LOG_DIR/test_runs/$TIMESTAMP"

# Create log directories
mkdir -p "$TEST_LOG_DIR"
mkdir -p "$TEST_LOG_DIR/individual_tests"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Main test log file
MAIN_LOG="$TEST_LOG_DIR/test_suite_summary.log"

echo -e "${BLUE}=== DX8GL Automated Test Suite ===${NC}" | tee "$MAIN_LOG"
echo "Project root: $PROJECT_ROOT" | tee -a "$MAIN_LOG"
echo "Build directory: $BUILD_DIR" | tee -a "$MAIN_LOG"
echo "Log directory: $TEST_LOG_DIR" | tee -a "$MAIN_LOG"
echo "Timestamp: $TIMESTAMP" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Build directory not found!${NC}"
    echo "Please run: ./scripts/compile.sh all"
    exit 1
fi

# Function to run test with timeout and capture output
run_test() {
    local test_name=$1
    local test_path=$2
    local timeout=${3:-30}
    
    # Log file for this specific test
    local log_file="$TEST_LOG_DIR/individual_tests/${test_name}.log"
    
    printf "%-40s" "Running $test_name..." | tee -a "$MAIN_LOG"
    
    if [ ! -f "$test_path" ]; then
        echo -e "${YELLOW}SKIPPED${NC} (not built)" | tee -a "$MAIN_LOG"
        echo "[SKIPPED] Test executable not found: $test_path" >> "$log_file"
        return 0
    fi
    
    # Set library paths for OSMesa and LLVM
    export LD_LIBRARY_PATH="$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:$BUILD_DIR/ext/dx8gl:$LD_LIBRARY_PATH"
    
    # Add test header to log file
    echo "=================================================================================" > "$log_file"
    echo "Test: $test_name" >> "$log_file"
    echo "Path: $test_path" >> "$log_file"
    echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')" >> "$log_file"
    echo "Timeout: ${timeout}s" >> "$log_file"
    echo "=================================================================================" >> "$log_file"
    echo "" >> "$log_file"
    
    # Run test with timeout and capture all output
    local start_time=$(date +%s)
    if timeout $timeout "$test_path" >> "$log_file" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        echo -e "${GREEN}PASSED${NC} (${duration}s)" | tee -a "$MAIN_LOG"
        echo "" >> "$log_file"
        echo "=================================================================================" >> "$log_file"
        echo "RESULT: PASSED" >> "$log_file"
        echo "Duration: ${duration} seconds" >> "$log_file"
        echo "=================================================================================" >> "$log_file"
        
        # Also save a summary in the main log
        echo "  [PASS] $test_name (${duration}s) - Full log: $log_file" >> "$MAIN_LOG"
        return 0
    else
        local exit_code=$?
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        if [ $exit_code -eq 124 ]; then
            echo -e "${RED}TIMEOUT${NC} (>${timeout}s)" | tee -a "$MAIN_LOG"
            echo "" >> "$log_file"
            echo "=================================================================================" >> "$log_file"
            echo "RESULT: TIMEOUT" >> "$log_file"
            echo "Duration: >${timeout} seconds" >> "$log_file"
            echo "=================================================================================" >> "$log_file"
        else
            echo -e "${RED}FAILED${NC} (exit: $exit_code, ${duration}s)" | tee -a "$MAIN_LOG"
            echo "" >> "$log_file"
            echo "=================================================================================" >> "$log_file"
            echo "RESULT: FAILED" >> "$log_file"
            echo "Exit Code: $exit_code" >> "$log_file"
            echo "Duration: ${duration} seconds" >> "$log_file"
            echo "=================================================================================" >> "$log_file"
        fi
        
        # Show last few lines in console
        echo "  Last 10 lines of output:" | tee -a "$MAIN_LOG"
        tail -n 10 "$log_file" | sed 's/^/    /' | tee -a "$MAIN_LOG"
        echo "  Full log saved to: $log_file" | tee -a "$MAIN_LOG"
        
        # Save failure summary in main log
        echo "  [FAIL] $test_name (exit: $exit_code, ${duration}s) - Full log: $log_file" >> "$MAIN_LOG"
        return 1
    fi
}

# Function to build tests
build_tests() {
    echo -e "${BLUE}Building dx8gl library and tests...${NC}"
    cd "$BUILD_DIR"
    
    # Build dx8gl library first
    if ! make dx8gl -j$(nproc) 2>&1 | tail -5; then
        echo -e "${RED}Failed to build dx8gl library${NC}"
        exit 1
    fi
    
    # Build test executables
    cd "$BUILD_DIR/ext/dx8gl/test"
    if ! make -j$(nproc) 2>&1 | tail -5; then
        echo -e "${YELLOW}Warning: Some tests may have failed to build${NC}"
    fi
    
    echo ""
}

# Parse command line arguments
RUN_COVERAGE=false
RUN_VALGRIND=false
FILTER=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --coverage)
            RUN_COVERAGE=true
            shift
            ;;
        --valgrind)
            RUN_VALGRIND=true
            shift
            ;;
        --filter)
            FILTER="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --coverage    Generate code coverage report"
            echo "  --valgrind    Run tests with Valgrind memory checking"
            echo "  --filter STR  Only run tests matching STR"
            echo "  --help        Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Build tests
build_tests

# Track test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0
FAILED_LIST=""

echo -e "${BLUE}=== Running Unit Tests ===${NC}"
echo ""

# Change to test directory
cd "$BUILD_DIR/ext/dx8gl/test"

# Get list of test executables
TEST_EXECUTABLES=$(ls test_* 2>/dev/null | grep -v "\.cpp" | sort)

if [ -n "$FILTER" ]; then
    TEST_EXECUTABLES=$(echo "$TEST_EXECUTABLES" | grep "$FILTER")
    echo "Filter applied: '$FILTER'"
    echo ""
fi

# Run each test
for test_exe in $TEST_EXECUTABLES; do
    if [ -x "$test_exe" ]; then
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
        
        if [ "$RUN_VALGRIND" = true ]; then
            # Run with Valgrind
            printf "%-40s" "Valgrind: $test_exe..."
            export LD_LIBRARY_PATH="$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:$BUILD_DIR/ext/dx8gl:$LD_LIBRARY_PATH"
            
            if valgrind --leak-check=full --error-exitcode=1 --quiet \
                       --suppressions="$PROJECT_ROOT/ext/dx8gl/dx8gl.supp" \
                       "./$test_exe" > /tmp/valgrind_$test_exe.log 2>&1; then
                echo -e "${GREEN}NO LEAKS${NC}"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                echo -e "${RED}MEMORY ISSUES${NC}"
                FAILED_TESTS=$((FAILED_TESTS + 1))
                FAILED_LIST="$FAILED_LIST\n  - $test_exe (memory)"
            fi
        else
            # Normal run
            if run_test "$test_exe" "./$test_exe"; then
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                FAILED_TESTS=$((FAILED_TESTS + 1))
                FAILED_LIST="$FAILED_LIST\n  - $test_exe"
            fi
        fi
    fi
done

# Run CTest suite if no filter is applied
if [ -z "$FILTER" ]; then
    echo "" | tee -a "$MAIN_LOG"
    echo -e "${BLUE}=== Running CTest Suite ===${NC}" | tee -a "$MAIN_LOG"
    cd "$BUILD_DIR/ext/dx8gl"
    
    # CTest log file
    CTEST_LOG="$TEST_LOG_DIR/ctest_results.log"
    
    # Count CTest tests
    CTEST_COUNT=$(ctest -N | grep "Test #" | wc -l)
    echo "Found $CTEST_COUNT tests registered with CTest" | tee -a "$MAIN_LOG"
    
    echo "=================================================================================" > "$CTEST_LOG"
    echo "CTest Suite Execution" >> "$CTEST_LOG"
    echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')" >> "$CTEST_LOG"
    echo "Test Count: $CTEST_COUNT" >> "$CTEST_LOG"
    echo "=================================================================================" >> "$CTEST_LOG"
    echo "" >> "$CTEST_LOG"
    
    if ctest --output-on-failure --timeout 30 >> "$CTEST_LOG" 2>&1; then
        echo -e "${GREEN}CTest suite PASSED${NC}" | tee -a "$MAIN_LOG"
        echo "  CTest results saved to: $CTEST_LOG" | tee -a "$MAIN_LOG"
    else
        echo -e "${RED}CTest suite had failures${NC}" | tee -a "$MAIN_LOG"
        echo "Last 20 lines of output:" | tee -a "$MAIN_LOG"
        tail -n 20 "$CTEST_LOG" | sed 's/^/  /' | tee -a "$MAIN_LOG"
        echo "  Full CTest log saved to: $CTEST_LOG" | tee -a "$MAIN_LOG"
    fi
fi

# Generate coverage report if requested
if [ "$RUN_COVERAGE" = true ]; then
    echo ""
    echo -e "${BLUE}=== Generating Coverage Report ===${NC}"
    cd "$BUILD_DIR"
    
    if command -v lcov &> /dev/null; then
        lcov --capture --directory . --output-file coverage.info 2>/dev/null
        lcov --remove coverage.info '/usr/*' '*/ext/glm/*' '*/build/*' --output-file coverage_filtered.info 2>/dev/null
        
        if command -v genhtml &> /dev/null; then
            genhtml coverage_filtered.info --output-directory coverage_report 2>/dev/null
            echo "Coverage report generated in: $BUILD_DIR/coverage_report/index.html"
        else
            echo -e "${YELLOW}genhtml not found, skipping HTML report${NC}"
        fi
    else
        echo -e "${YELLOW}lcov not found, skipping coverage report${NC}"
    fi
fi

# Generate test report
echo "" | tee -a "$MAIN_LOG"
echo -e "${BLUE}=== Test Report ===${NC}" | tee -a "$MAIN_LOG"
echo "Total tests run: $TOTAL_TESTS" | tee -a "$MAIN_LOG"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}" | tee -a "$MAIN_LOG"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}" | tee -a "$MAIN_LOG"

if [ $SKIPPED_TESTS -gt 0 ]; then
    echo -e "Skipped: ${YELLOW}$SKIPPED_TESTS${NC}" | tee -a "$MAIN_LOG"
fi

# Save final summary to main log without color codes
{
    echo ""
    echo "=================================================================================" 
    echo "FINAL TEST SUMMARY"
    echo "=================================================================================" 
    echo "Total tests run: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $FAILED_TESTS"
    echo "Skipped: $SKIPPED_TESTS"
    echo "Test completion time: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "All logs saved to: $TEST_LOG_DIR"
    echo "=================================================================================" 
} >> "$MAIN_LOG"

if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "\nFailed tests:$FAILED_LIST" | tee -a "$MAIN_LOG"
    echo "" | tee -a "$MAIN_LOG"
    echo "To debug a failed test, run:" | tee -a "$MAIN_LOG"
    echo "  export LD_LIBRARY_PATH=$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:\$LD_LIBRARY_PATH" | tee -a "$MAIN_LOG"
    echo "  gdb $BUILD_DIR/ext/dx8gl/test/<test_name>" | tee -a "$MAIN_LOG"
    echo "" | tee -a "$MAIN_LOG"
    echo "All test logs saved in: $TEST_LOG_DIR" | tee -a "$MAIN_LOG"
    
    # Create a symlink to latest failed test run
    ln -sfn "$TEST_LOG_DIR" "$LOG_DIR/test_runs/latest_failed"
    echo "Latest failed test logs linked at: $LOG_DIR/test_runs/latest_failed" | tee -a "$MAIN_LOG"
    
    exit 1
else
    echo "" | tee -a "$MAIN_LOG"
    echo -e "${GREEN}All tests PASSED!${NC}" | tee -a "$MAIN_LOG"
    
    # Create a symlink to latest successful test run
    ln -sfn "$TEST_LOG_DIR" "$LOG_DIR/test_runs/latest_passed"
    echo "Latest passed test logs linked at: $LOG_DIR/test_runs/latest_passed" | tee -a "$MAIN_LOG"
    
    # Show performance summary if available
    if [ -f "/tmp/test_performance.log" ]; then
        echo "" | tee -a "$MAIN_LOG"
        echo "Performance Summary:" | tee -a "$MAIN_LOG"
        grep "Average" /tmp/test_performance.log | tail -5 | tee -a "$MAIN_LOG"
    fi
    
    echo "" | tee -a "$MAIN_LOG"
    echo "All test logs saved in: $TEST_LOG_DIR" | tee -a "$MAIN_LOG"
    
    exit 0
fi