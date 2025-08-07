# DX8GL Test Automation Guide

## Overview
The dx8gl library has a comprehensive test suite located in `ext/dx8gl/test/` that uses CMake's CTest framework for test automation. This guide explains how to use, expand, and automate the testing infrastructure.

## Current Test Infrastructure

### Test Categories
1. **Core API Tests** - Verify DirectX 8 API implementation
2. **Shader Tests** - Test shader translation and compilation
3. **Resource Tests** - Test textures, buffers, and surfaces
4. **Backend Tests** - Test OSMesa, EGL, and WebGPU backends
5. **State Management Tests** - Verify render state handling
6. **Performance Tests** - Measure and validate performance

### Existing Test Files
- `test_dx8gl_core_api.cpp` - Core DirectX 8 API functionality
- `test_shader_translator.cpp` - DirectX shader to GLSL translation
- `test_device_reset.cpp` - Device reset and resource recreation
- `test_cube_texture.cpp` - Cube texture functionality
- `test_backend_selection.cpp` - Runtime backend switching
- `test_render_states.cpp` - Render state management
- `test_alpha_blending.cpp` - Alpha blending operations
- And many more...

## Running Tests

### Quick Test Run
```bash
# From project root
cd build/ext/dx8gl
make test

# Or use CTest directly
ctest --output-on-failure

# Run specific test
ctest -R ShaderTranslatorTest -V

# Run with timeout
ctest --timeout 30
```

### Using the Test Runner Script
```bash
# From dx8gl directory
./run_tests.sh

# This script:
# 1. Builds all test targets
# 2. Runs CTest suite
# 3. Executes shader feature tests
# 4. Tests backend selection
# 5. Validates framebuffer correctness
```

### Run Tests with Proper Library Paths
```bash
# Set library paths for OSMesa/LLVM
export LD_LIBRARY_PATH=$PWD/build/llvm-install/lib:$PWD/build/mesa-install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

# Run individual test
./build/ext/dx8gl/test/test_render_states
```

## Adding New Tests

### 1. Create Test File
Create a new test file in `ext/dx8gl/test/`:

```cpp
// test_com_wrapper.cpp
#include <iostream>
#include <cassert>
#include "d3d8.h"
#include "dx8gl.h"

int main() {
    std::cout << "Testing COM wrapper functionality..." << std::endl;
    
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    if (dx8gl_init(&config) != 0) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return 1;
    }
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    assert(d3d8 != nullptr);
    
    // Test reference counting
    d3d8->AddRef();
    ULONG refCount = d3d8->Release();
    assert(refCount == 1);
    
    // Test QueryInterface
    void* pUnknown = nullptr;
    HRESULT hr = d3d8->QueryInterface(IID_IUnknown, &pUnknown);
    assert(SUCCEEDED(hr));
    assert(pUnknown != nullptr);
    
    // Cleanup
    d3d8->Release();
    dx8gl_cleanup();
    
    std::cout << "COM wrapper tests PASSED!" << std::endl;
    return 0;
}
```

### 2. Add to CMakeLists.txt
Add your test to `ext/dx8gl/test/CMakeLists.txt`:

```cmake
# COM Wrapper test
add_executable(test_com_wrapper 
    test_com_wrapper.cpp
)

target_include_directories(test_com_wrapper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

if(TARGET dx8gl)
    target_link_libraries(test_com_wrapper PRIVATE dx8gl)
    add_test(NAME COMWrapperTest COMMAND test_com_wrapper)
else()
    message(WARNING "dx8gl target not available, skipping test_com_wrapper")
    set_target_properties(test_com_wrapper PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
```

### 3. Build and Register Test
```bash
# Rebuild with new test
cd build
cmake ..
make test_com_wrapper

# Run the new test
ctest -R COMWrapperTest -V
```

## Test Automation Scripts

### Create Continuous Test Runner
Create `scripts/run_dx8gl_tests.sh`:

```bash
#!/bin/bash
# Automated test runner for dx8gl

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=== DX8GL Automated Test Suite ==="
echo "Build directory: $BUILD_DIR"

# Function to run test with timeout and capture output
run_test() {
    local test_name=$1
    local test_path=$2
    local timeout=${3:-30}
    
    echo -n "Running $test_name... "
    
    if [ ! -f "$test_path" ]; then
        echo -e "${YELLOW}SKIPPED${NC} (not built)"
        return 0
    fi
    
    # Set library paths
    export LD_LIBRARY_PATH="$BUILD_DIR/llvm-install/lib:$BUILD_DIR/mesa-install/lib/x86_64-linux-gnu:$BUILD_DIR/ext/dx8gl:$LD_LIBRARY_PATH"
    
    # Run test with timeout
    if timeout $timeout "$test_path" > "/tmp/${test_name}.log" 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        echo "  Output saved to /tmp/${test_name}.log"
        tail -n 10 "/tmp/${test_name}.log"
        return 1
    fi
}

# Build tests first
echo "Building dx8gl tests..."
cd "$BUILD_DIR"
make -j$(nproc) dx8gl 2>&1 | tail -5

# Build all tests
cd "$BUILD_DIR/ext/dx8gl/test"
make -j$(nproc) 2>&1 | tail -5

# Track test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
FAILED_LIST=""

echo -e "\n=== Running Unit Tests ===\n"

# Run each test
for test_exe in test_*; do
    if [ -x "$test_exe" ]; then
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
        if run_test "$test_exe" "./$test_exe"; then
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            FAILED_TESTS=$((FAILED_TESTS + 1))
            FAILED_LIST="$FAILED_LIST\n  - $test_exe"
        fi
    fi
done

# Run CTest suite
echo -e "\n=== Running CTest Suite ===\n"
cd "$BUILD_DIR/ext/dx8gl"
if ctest --output-on-failure --timeout 30 > /tmp/ctest_results.log 2>&1; then
    echo -e "${GREEN}CTest suite PASSED${NC}"
else
    echo -e "${RED}CTest suite had failures${NC}"
    tail -n 20 /tmp/ctest_results.log
fi

# Generate test report
echo -e "\n=== Test Report ==="
echo "Total tests run: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "\nFailed tests:$FAILED_LIST"
    exit 1
else
    echo -e "\n${GREEN}All tests PASSED!${NC}"
    exit 0
fi
```

### Create Test Coverage Script
Create `scripts/test_coverage.sh`:

```bash
#!/bin/bash
# Generate test coverage report for dx8gl

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build_coverage"

echo "=== DX8GL Test Coverage Analysis ==="

# Create coverage build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with coverage flags
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage"

# Build dx8gl with coverage
make dx8gl -j$(nproc)

# Build and run tests
cd ext/dx8gl/test
make -j$(nproc)

# Run all tests to generate coverage data
for test_exe in test_*; do
    if [ -x "$test_exe" ]; then
        echo "Running $test_exe for coverage..."
        ./"$test_exe" || true
    fi
done

# Generate coverage report
cd "$BUILD_DIR"
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/ext/glm/*' '*/build/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report

echo "Coverage report generated in: $BUILD_DIR/coverage_report/index.html"
```

## Continuous Integration

### GitHub Actions Workflow
Create `.github/workflows/dx8gl_tests.yml`:

```yaml
name: DX8GL Test Suite

on:
  push:
    paths:
      - 'ext/dx8gl/**'
      - 'scripts/compile.sh'
  pull_request:
    paths:
      - 'ext/dx8gl/**'

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          cmake ninja-build \
          libx11-dev libxext-dev \
          libsdl2-dev \
          libegl1-mesa-dev \
          mesa-utils
    
    - name: Build LLVM and Mesa
      run: |
        ./scripts/compile.sh llvm
        ./scripts/compile.sh mesa
    
    - name: Build dx8gl
      run: |
        cd build
        make dx8gl -j$(nproc)
    
    - name: Build tests
      run: |
        cd build/ext/dx8gl/test
        make -j$(nproc)
    
    - name: Run tests
      run: |
        cd build/ext/dx8gl
        ctest --output-on-failure --timeout 30
    
    - name: Upload test results
      if: always()
      uses: actions/upload-artifact@v3
      with:
        name: test-results
        path: |
          build/ext/dx8gl/Testing/
          /tmp/*.log
```

## Test Validation Checklist

### For Each New Feature
- [ ] Unit test for the feature
- [ ] Integration test with existing components
- [ ] Performance benchmark (if applicable)
- [ ] Multi-threaded safety test (if applicable)
- [ ] Memory leak check with Valgrind
- [ ] Cross-backend compatibility test

### Test Quality Metrics
- **Coverage**: Aim for >80% code coverage
- **Performance**: Tests should complete in <1 second each
- **Isolation**: Tests should not depend on each other
- **Repeatability**: Tests must produce consistent results
- **Documentation**: Each test should have clear purpose

## Memory Leak Testing

### Using Valgrind
```bash
# Run test with memory leak detection
valgrind --leak-check=full --show-leak-kinds=all \
    ./build/ext/dx8gl/test/test_device_reset

# Run with suppression file for known issues
valgrind --leak-check=full --suppressions=dx8gl.supp \
    ./build/ext/dx8gl/test/test_com_wrapper
```

### Create Suppression File
Create `ext/dx8gl/dx8gl.supp`:
```
{
   OpenGL_Driver_Leaks
   Memcheck:Leak
   ...
   obj:*/libGL.so*
}

{
   Mesa_Init_Leaks
   Memcheck:Leak
   ...
   obj:*/libOSMesa.so*
}
```

## Performance Testing

### Create Benchmark Test
```cpp
// test_performance.cpp
#include <chrono>
#include <iostream>
#include "d3d8.h"
#include "dx8gl.h"

int main() {
    // Initialize
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    dx8gl_init(&config);
    
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    IDirect3DDevice8* device = nullptr;
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    d3d8->CreateDevice(0, D3DDEVTYPE_HAL, nullptr,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &pp, &device);
    
    // Benchmark state changes
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; i++) {
        device->SetRenderState(D3DRS_ZENABLE, TRUE);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "10000 state changes took: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average per state change: " << duration.count() / 10000.0 << " microseconds" << std::endl;
    
    // Cleanup
    device->Release();
    d3d8->Release();
    dx8gl_cleanup();
    
    return 0;
}
```

## Test Organization Best Practices

### Directory Structure
```
ext/dx8gl/test/
├── unit/           # Unit tests for individual components
├── integration/    # Integration tests
├── performance/    # Performance benchmarks
├── regression/     # Regression tests for bug fixes
├── fixtures/       # Test data and fixtures
└── utils/          # Test utility functions
```

### Naming Convention
- `test_<component>_<feature>.cpp` - For unit tests
- `bench_<component>.cpp` - For performance tests
- `regress_<bug_id>.cpp` - For regression tests

### Test Documentation
Each test file should include:
```cpp
/**
 * @file test_com_wrapper.cpp
 * @brief Tests for COM wrapper thread safety and reference counting
 * 
 * Tests:
 * - Reference counting correctness
 * - Thread-safe AddRef/Release
 * - QueryInterface implementation
 * - Resource cleanup on destruction
 * 
 * @date 2025-08-07
 */
```

## Debugging Failed Tests

### Enable Debug Output
```bash
# Set environment variables for debug output
export DX8GL_LOG_LEVEL=DEBUG
export DX8GL_LOG_FILE=/tmp/dx8gl_test.log

# Run test with debugging
./build/ext/dx8gl/test/test_device_reset

# Check log file
cat /tmp/dx8gl_test.log
```

### Use GDB for Debugging
```bash
# Debug a failing test
gdb ./build/ext/dx8gl/test/test_com_wrapper
(gdb) run
(gdb) bt  # Get backtrace when it crashes
(gdb) frame 2  # Go to specific frame
(gdb) info locals  # Show local variables
```

## Next Steps

1. **Implement Missing Tests**:
   - COM wrapper thread safety
   - Resource leak detection
   - Backend fallback scenarios
   - Shader compilation edge cases

2. **Automate Test Execution**:
   - Set up nightly test runs
   - Add pre-commit hooks
   - Integrate with CI/CD pipeline

3. **Improve Test Coverage**:
   - Add tests for error conditions
   - Test resource limits
   - Validate performance requirements

4. **Documentation**:
   - Document test dependencies
   - Create test writing guide
   - Maintain test status dashboard