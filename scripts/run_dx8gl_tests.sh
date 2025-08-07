#!/bin/bash
# Automated test runner for dx8gl library
# This script builds and runs all dx8gl tests with proper library paths

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== DX8GL Automated Test Suite ===${NC}"
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo ""

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
    
    printf "%-40s" "Running $test_name..."
    
    if [ ! -f "$test_path" ]; then
        echo -e "${YELLOW}SKIPPED${NC} (not built)"
        return 0
    fi
    
    # Set library paths for OSMesa and LLVM
    export LD_LIBRARY_PATH="$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:$BUILD_DIR/ext/dx8gl:$LD_LIBRARY_PATH"
    
    # Create temp file for output
    local log_file="/tmp/${test_name}_$(date +%s).log"
    
    # Run test with timeout
    if timeout $timeout "$test_path" > "$log_file" 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        rm -f "$log_file"
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo -e "${RED}TIMEOUT${NC}"
        else
            echo -e "${RED}FAILED${NC} (exit code: $exit_code)"
        fi
        echo "  Last 10 lines of output:"
        tail -n 10 "$log_file" | sed 's/^/    /'
        echo "  Full output saved to: $log_file"
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
    echo ""
    echo -e "${BLUE}=== Running CTest Suite ===${NC}"
    cd "$BUILD_DIR/ext/dx8gl"
    
    # Count CTest tests
    CTEST_COUNT=$(ctest -N | grep "Test #" | wc -l)
    echo "Found $CTEST_COUNT tests registered with CTest"
    
    if ctest --output-on-failure --timeout 30 > /tmp/ctest_results.log 2>&1; then
        echo -e "${GREEN}CTest suite PASSED${NC}"
    else
        echo -e "${RED}CTest suite had failures${NC}"
        echo "Last 20 lines of output:"
        tail -n 20 /tmp/ctest_results.log | sed 's/^/  /'
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
echo ""
echo -e "${BLUE}=== Test Report ===${NC}"
echo "Total tests run: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $SKIPPED_TESTS -gt 0 ]; then
    echo -e "Skipped: ${YELLOW}$SKIPPED_TESTS${NC}"
fi

if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "\nFailed tests:$FAILED_LIST"
    echo ""
    echo "To debug a failed test, run:"
    echo "  export LD_LIBRARY_PATH=$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:\$LD_LIBRARY_PATH"
    echo "  gdb $BUILD_DIR/ext/dx8gl/test/<test_name>"
    exit 1
else
    echo ""
    echo -e "${GREEN}All tests PASSED!${NC}"
    
    # Show performance summary if available
    if [ -f "/tmp/test_performance.log" ]; then
        echo ""
        echo "Performance Summary:"
        grep "Average" /tmp/test_performance.log | tail -5
    fi
    
    exit 0
fi