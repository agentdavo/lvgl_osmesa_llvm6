#!/bin/bash
# Multi-backend test runner for dx8gl library
# This script runs all tests with each available backend (OSMesa, EGL, WebGPU)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== DX8GL Multi-Backend Test Suite ===${NC}"
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo ""

# Default backends to test
BACKENDS_TO_TEST="osmesa egl webgpu"
FILTER=""
RUN_VALGRIND=false
RUN_COVERAGE=false
UPDATE_GOLDEN=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --backends)
            BACKENDS_TO_TEST="$2"
            shift 2
            ;;
        --filter)
            FILTER="$2"
            shift 2
            ;;
        --valgrind)
            RUN_VALGRIND=true
            shift
            ;;
        --coverage)
            RUN_COVERAGE=true
            shift
            ;;
        --update-golden)
            UPDATE_GOLDEN=true
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --backends LIST     Space-separated list of backends (default: 'osmesa egl webgpu')"
            echo "  --filter STR        Only run tests matching STR"
            echo "  --valgrind          Run tests with Valgrind memory checking"
            echo "  --coverage          Generate code coverage report"
            echo "  --update-golden     Update golden images for rendering tests"
            echo "  --help              Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                                    # Run all tests on all backends"
            echo "  $0 --backends 'osmesa egl'            # Test only OSMesa and EGL"
            echo "  $0 --filter shader --backends osmesa  # Run shader tests on OSMesa only"
            echo "  $0 --update-golden                    # Regenerate golden images"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Build directory not found!${NC}"
    echo "Please run: ./scripts/compile.sh all"
    exit 1
fi

# Function to check if backend is available
check_backend_available() {
    local backend=$1
    case $backend in
        osmesa)
            # OSMesa is always available if built
            return 0
            ;;
        egl)
            # Check if EGL backend was built
            if [ -f "$BUILD_DIR/ext/dx8gl/libdx8gl.a" ]; then
                # Check if EGL libraries are available
                if ldconfig -p | grep -q libEGL.so; then
                    return 0
                fi
            fi
            return 1
            ;;
        webgpu)
            # Check if WebGPU backend was built
            if [ -f "$BUILD_DIR/ext/dx8gl/libdx8gl.a" ]; then
                # WebGPU availability depends on build flags
                if ldd "$BUILD_DIR/ext/dx8gl/libdx8gl.a" 2>/dev/null | grep -q webgpu; then
                    return 0
                fi
            fi
            return 1
            ;;
        *)
            return 1
            ;;
    esac
}

# Function to run a single test with specific backend
run_test_with_backend() {
    local test_path=$1
    local test_name=$2
    local backend=$3
    local timeout=${4:-30}
    
    # Set backend environment variable
    export DX8GL_BACKEND=$backend
    
    # Set library paths
    export LD_LIBRARY_PATH="$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:$BUILD_DIR/ext/dx8gl:$LD_LIBRARY_PATH"
    
    # Set golden image update flag if requested
    if [ "$UPDATE_GOLDEN" = true ]; then
        export UPDATE_GOLDEN_IMAGES=1
    fi
    
    # Create temp file for output
    local log_file="/tmp/${test_name}_${backend}_$(date +%s).log"
    
    # Run test with timeout
    if timeout $timeout "$test_path" > "$log_file" 2>&1; then
        echo -e "    ${GREEN}✓${NC} $backend"
        rm -f "$log_file"
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo -e "    ${RED}✗${NC} $backend (timeout)"
        else
            echo -e "    ${RED}✗${NC} $backend (exit: $exit_code)"
        fi
        
        # Show brief error if not too verbose
        if grep -q "GTEST_SKIP" "$log_file"; then
            echo -e "      ${YELLOW}Skipped:${NC} $(grep "GTEST_SKIP" "$log_file" | head -1 | sed 's/.*GTEST_SKIP.*] //')"
        else
            echo "      Last error: $(grep -E "FAIL|ERROR|ASSERT" "$log_file" | tail -1)"
        fi
        return 1
    fi
}

# Function to build tests if needed
build_tests() {
    echo -e "${BLUE}Checking dx8gl library and tests...${NC}"
    cd "$BUILD_DIR"
    
    # Check if tests are built
    if [ ! -d "ext/dx8gl/test" ] || [ -z "$(ls ext/dx8gl/test/test_* 2>/dev/null | grep -v '\.cpp')" ]; then
        echo "Building tests..."
        if ! make -C ext/dx8gl/test -j$(nproc) 2>&1 | tail -5; then
            echo -e "${YELLOW}Warning: Some tests may have failed to build${NC}"
        fi
    fi
    echo ""
}

# Build tests
build_tests

# Statistics tracking
declare -A BACKEND_STATS
declare -A TEST_RESULTS

# Initialize backend statistics
for backend in $BACKENDS_TO_TEST; do
    BACKEND_STATS["${backend}_total"]=0
    BACKEND_STATS["${backend}_passed"]=0
    BACKEND_STATS["${backend}_failed"]=0
    BACKEND_STATS["${backend}_skipped"]=0
done

echo -e "${BLUE}=== Testing Backends ===${NC}"
echo "Backends to test: $BACKENDS_TO_TEST"
echo ""

# Check which backends are available
echo "Checking backend availability:"
AVAILABLE_BACKENDS=""
for backend in $BACKENDS_TO_TEST; do
    printf "  %-10s: " "$backend"
    if check_backend_available "$backend"; then
        echo -e "${GREEN}Available${NC}"
        AVAILABLE_BACKENDS="$AVAILABLE_BACKENDS $backend"
    else
        echo -e "${YELLOW}Not available${NC}"
    fi
done
echo ""

if [ -z "$AVAILABLE_BACKENDS" ]; then
    echo -e "${RED}No backends available for testing!${NC}"
    exit 1
fi

# Change to test directory
cd "$BUILD_DIR/ext/dx8gl/test"

# Get list of test executables
TEST_EXECUTABLES=$(ls test_* 2>/dev/null | grep -v "\.cpp" | sort)

if [ -n "$FILTER" ]; then
    TEST_EXECUTABLES=$(echo "$TEST_EXECUTABLES" | grep "$FILTER")
    echo "Filter applied: '$FILTER'"
    echo ""
fi

# Count total tests
TOTAL_TEST_COUNT=$(echo "$TEST_EXECUTABLES" | wc -w)
echo -e "${BLUE}=== Running $TOTAL_TEST_COUNT tests on $(echo $AVAILABLE_BACKENDS | wc -w) backends ===${NC}"
echo ""

# Run tests for each backend
for backend in $AVAILABLE_BACKENDS; do
    echo -e "${CYAN}--- Backend: ${backend^^} ---${NC}"
    
    for test_exe in $TEST_EXECUTABLES; do
        if [ -x "$test_exe" ]; then
            printf "%-40s" "$test_exe"
            
            BACKEND_STATS["${backend}_total"]=$((BACKEND_STATS["${backend}_total"] + 1))
            
            if run_test_with_backend "./$test_exe" "$test_exe" "$backend"; then
                BACKEND_STATS["${backend}_passed"]=$((BACKEND_STATS["${backend}_passed"] + 1))
                TEST_RESULTS["${test_exe}_${backend}"]="PASS"
            else
                BACKEND_STATS["${backend}_failed"]=$((BACKEND_STATS["${backend}_failed"] + 1))
                TEST_RESULTS["${test_exe}_${backend}"]="FAIL"
            fi
        fi
    done
    echo ""
done

# Generate comparison matrix
echo -e "${BLUE}=== Test Comparison Matrix ===${NC}"
echo ""

# Print header
printf "%-40s" "Test"
for backend in $AVAILABLE_BACKENDS; do
    printf "%-10s" "${backend^^}"
done
echo ""
echo "$(printf '%.0s-' {1..80})"

# Print test results
for test_exe in $TEST_EXECUTABLES; do
    printf "%-40s" "$test_exe"
    for backend in $AVAILABLE_BACKENDS; do
        result="${TEST_RESULTS["${test_exe}_${backend}"]}"
        case $result in
            PASS)
                printf "${GREEN}%-10s${NC}" "✓"
                ;;
            FAIL)
                printf "${RED}%-10s${NC}" "✗"
                ;;
            *)
                printf "${YELLOW}%-10s${NC}" "-"
                ;;
        esac
    done
    echo ""
done

# Print summary statistics
echo ""
echo -e "${BLUE}=== Backend Summary ===${NC}"
echo ""

for backend in $AVAILABLE_BACKENDS; do
    total=${BACKEND_STATS["${backend}_total"]}
    passed=${BACKEND_STATS["${backend}_passed"]}
    failed=${BACKEND_STATS["${backend}_failed"]}
    
    if [ $total -gt 0 ]; then
        pass_rate=$((passed * 100 / total))
    else
        pass_rate=0
    fi
    
    echo -e "${CYAN}$backend:${NC}"
    echo "  Total:  $total"
    echo -e "  Passed: ${GREEN}$passed${NC}"
    echo -e "  Failed: ${RED}$failed${NC}"
    echo "  Pass rate: ${pass_rate}%"
    echo ""
done

# Identify backend-specific failures
echo -e "${BLUE}=== Backend-Specific Issues ===${NC}"
echo ""

has_issues=false
for test_exe in $TEST_EXECUTABLES; do
    failures=""
    for backend in $AVAILABLE_BACKENDS; do
        if [ "${TEST_RESULTS["${test_exe}_${backend}"]}" = "FAIL" ]; then
            failures="$failures $backend"
        fi
    done
    
    if [ -n "$failures" ]; then
        # Check if it fails on all backends or just some
        fail_count=$(echo $failures | wc -w)
        backend_count=$(echo $AVAILABLE_BACKENDS | wc -w)
        
        if [ $fail_count -eq $backend_count ]; then
            echo -e "${RED}$test_exe fails on ALL backends${NC}"
        else
            echo -e "${YELLOW}$test_exe fails on:${NC}$failures"
        fi
        has_issues=true
    fi
done

if [ "$has_issues" = false ]; then
    echo -e "${GREEN}No backend-specific issues found!${NC}"
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
        fi
    else
        echo -e "${YELLOW}lcov not found, skipping coverage report${NC}"
    fi
fi

# Final summary
echo ""
echo -e "${BLUE}=== Overall Summary ===${NC}"

total_tests=0
total_passed=0
total_failed=0

for backend in $AVAILABLE_BACKENDS; do
    total_tests=$((total_tests + BACKEND_STATS["${backend}_total"]))
    total_passed=$((total_passed + BACKEND_STATS["${backend}_passed"]))
    total_failed=$((total_failed + BACKEND_STATS["${backend}_failed"]))
done

echo "Total test runs: $total_tests"
echo -e "Total passed: ${GREEN}$total_passed${NC}"
echo -e "Total failed: ${RED}$total_failed${NC}"

if [ $total_failed -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ All tests passed on all backends!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}✗ Some tests failed. See details above.${NC}"
    echo ""
    echo "To debug a specific test on a backend:"
    echo "  export DX8GL_BACKEND=<backend>"
    echo "  export LD_LIBRARY_PATH=$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:\$LD_LIBRARY_PATH"
    echo "  gdb $BUILD_DIR/ext/dx8gl/test/<test_name>"
    exit 1
fi