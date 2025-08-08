# Test Logging Documentation

## Overview

The dx8gl test suite uses Google Test (gtest) framework and provides comprehensive logging to help debug test failures and track test history. All test runs are automatically logged to the `logs/` directory with timestamps and detailed output.

## Directory Structure

```
logs/
├── test_runs/                    # Full test suite runs
│   ├── YYYYMMDD_HHMMSS/         # Timestamped test run
│   │   ├── test_suite_summary.log
│   │   ├── individual_tests/     # Per-test logs
│   │   │   ├── test_name.log
│   │   │   └── ...
│   │   └── ctest_results.log
│   ├── latest_passed/            # Symlink to last successful run
│   └── latest_failed/            # Symlink to last failed run
└── gtest_runs/                   # Individual test runs
    ├── test_name_TIMESTAMP/      # Single test execution
    │   ├── console_output.log
    │   ├── gtest_output.log
    │   ├── test_results.xml
    │   └── summary.txt
    └── latest_test_name/         # Symlink to latest run
```

## Running Tests with Logging

### 1. Full Test Suite

Run all tests with automatic logging:

```bash
./scripts/run_dx8gl_tests.sh
```

Options:
- `--filter STR` - Only run tests matching pattern
- `--coverage` - Generate code coverage report
- `--valgrind` - Run with memory leak detection

Logs are saved to: `logs/test_runs/YYYYMMDD_HHMMSS/`

### 2. Individual Google Tests

Run specific tests with detailed logging:

```bash
./scripts/run_gtest_with_logs.sh test_d3dx_math
```

With filters:
```bash
./scripts/run_gtest_with_logs.sh test_d3dx_math --gtest_filter="*PlaneOperations*"
```

Common gtest options:
- `--gtest_list_tests` - List all tests without running
- `--gtest_filter=PATTERN` - Run only tests matching pattern
- `--gtest_repeat=N` - Run tests N times
- `--gtest_shuffle` - Randomize test order
- `--gtest_output=xml:file` - Generate XML test report
- `--gtest_break_on_failure` - Invoke debugger on failure

### 3. OSMesa Backend Testing

Run tests specifically with OSMesa backend:

```bash
export DX8GL_BACKEND=osmesa
./scripts/run_dx8gl_tests.sh
```

## Log File Contents

### test_suite_summary.log

Main summary with:
- Test execution timeline
- Pass/fail status for each test
- Links to individual test logs
- Final statistics

Example:
```
=== DX8GL Automated Test Suite ===
Project root: /home/djs/lvgl_osmesa_llvm6
Build directory: /home/djs/lvgl_osmesa_llvm6/build
Log directory: /home/djs/lvgl_osmesa_llvm6/logs/test_runs/20250808_095024
Timestamp: 20250808_095024

Running test_d3dx_math...                PASSED (1s)
  [PASS] test_d3dx_math (1s) - Full log: .../individual_tests/test_d3dx_math.log

=================================================================================
FINAL TEST SUMMARY
=================================================================================
Total tests run: 10
Passed: 8
Failed: 2
Skipped: 0
```

### Individual Test Logs

Each test gets a detailed log with:
- Full console output
- Google Test output
- Timing information
- Exit codes
- Error messages

Example structure:
```
=================================================================================
Test: test_d3dx_math
Path: /home/djs/lvgl_osmesa_llvm6/build/Release/test_d3dx_math
Timestamp: 2025-08-08 09:50:24
Timeout: 30s
=================================================================================

[Google Test Output]
Running main() from gtest_main.cc
[==========] Running 19 tests from 1 test suite.
[----------] 19 tests from D3DXMathTest
[ RUN      ] D3DXMathTest.MatrixIdentity
[       OK ] D3DXMathTest.MatrixIdentity (0 ms)
...

=================================================================================
RESULT: PASSED
Duration: 1 seconds
=================================================================================
```

### XML Test Reports

Google Test generates XML reports in JUnit format for CI integration:
- Located in `test_results.xml`
- Contains detailed test case information
- Can be parsed by CI systems

## Debugging Failed Tests

### 1. Finding Failed Test Logs

Latest failed run:
```bash
cd logs/test_runs/latest_failed/
cat test_suite_summary.log  # Overview
ls individual_tests/        # Find specific test
```

### 2. Analyzing Test Output

View full test output:
```bash
less logs/test_runs/latest_failed/individual_tests/test_name.log
```

Check for patterns:
```bash
grep -n "FAILED\|ERROR\|ASSERT" logs/test_runs/latest_failed/individual_tests/*.log
```

### 3. Running Failed Test with Debugger

The test logs provide the exact command to debug:
```bash
export LD_LIBRARY_PATH=/path/to/llvm/lib:/path/to/mesa/lib:$LD_LIBRARY_PATH
gdb /path/to/test/executable
```

## Test Categories

### Core Tests
- `test_dx8gl_core_api` - Core API functionality
- `test_backend_selection` - Backend selection logic
- `test_framebuffer_correctness` - Framebuffer operations

### Math & Utility Tests  
- `test_d3dx_math` - D3DX math functions
- `test_d3dx_color` - Color manipulation
- `test_matrix_pipeline` - Matrix transformations

### Shader Tests
- `test_shader_translator` - Shader translation
- `test_shader_program_linking` - Program linking
- `test_shader_constants` - Constant management
- `test_shader_cache_simple` - Shader caching

### Backend Tests
- `test_device_caps_parity` - Device capability comparison
- `test_cross_backend_rendering` - Cross-backend rendering
- `test_cross_backend_enhanced` - Advanced rendering
- `test_webgpu_async` - WebGPU async operations

## Best Practices

1. **Always check logs** - Even for passing tests, logs can reveal warnings
2. **Use filters** - Run specific tests during development
3. **Save baselines** - Keep logs from known-good runs for comparison
4. **Check timestamps** - Ensure you're looking at the correct test run
5. **Use symlinks** - `latest_passed` and `latest_failed` for quick access

## Environment Variables

- `DX8GL_BACKEND` - Select backend (osmesa, egl, webgpu, auto)
- `LD_LIBRARY_PATH` - Library paths for OSMesa/LLVM
- `GTEST_COLOR` - Enable/disable colored output (yes/no/auto)
- `GTEST_PRINT_TIME` - Print test execution time (0/1)

## Continuous Integration

The logging system is designed for CI integration:

1. XML reports for test results
2. Timestamped logs for history
3. Exit codes for success/failure
4. Machine-readable summary files

Example CI usage:
```yaml
- name: Run Tests
  run: ./scripts/run_dx8gl_tests.sh
  
- name: Upload Test Logs
  if: always()
  uses: actions/upload-artifact@v2
  with:
    name: test-logs
    path: logs/test_runs/latest_*/
```

## Troubleshooting

### Missing Logs
- Check write permissions on `logs/` directory
- Ensure disk space is available
- Verify script has execute permissions

### Incomplete Logs
- Test may have crashed - check core dumps
- Timeout may be too short - adjust in script
- Check system resources (memory, CPU)

### Log Rotation
- Old logs are not automatically deleted
- Implement rotation policy as needed:
  ```bash
  find logs/test_runs -mtime +30 -type d -exec rm -rf {} \;
  ```

## Summary

The test logging system provides:
- Complete test output capture
- Timestamped organization
- Easy debugging access
- CI/CD integration support
- Historical test tracking

All tests using Google Test automatically benefit from this logging infrastructure.