#!/bin/bash
# Script to run dx8_backend_test and compare outputs

echo "=== Running Backend Regression Test ==="

# Set library path
export LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu

# Create test directory if needed
mkdir -p build/backend_test_results

# Change to test directory
cd build/backend_test_results

# Run the test
echo "Running backend test..."
../../src/dx8_backend_test/dx8_backend_test

TEST_RESULT=$?

# Check outputs
if [ $TEST_RESULT -eq 0 ]; then
    echo ""
    echo "=== Test Results ==="
    
    # Check if output files exist
    if [ -f "backend_test_osmesa.ppm" ] && [ -f "backend_test_egl.ppm" ]; then
        echo "Both backend outputs generated successfully"
        
        # Get file sizes
        OSMESA_SIZE=$(stat -c%s "backend_test_osmesa.ppm" 2>/dev/null || stat -f%z "backend_test_osmesa.ppm" 2>/dev/null)
        EGL_SIZE=$(stat -c%s "backend_test_egl.ppm" 2>/dev/null || stat -f%z "backend_test_egl.ppm" 2>/dev/null)
        
        echo "OSMesa output size: $OSMESA_SIZE bytes"
        echo "EGL output size: $EGL_SIZE bytes"
        
        # Simple size comparison
        if [ "$OSMESA_SIZE" = "$EGL_SIZE" ]; then
            echo "Output file sizes match"
        else
            echo "Warning: Output file sizes differ"
        fi
        
        # Use ImageMagick compare if available
        if command -v compare &> /dev/null; then
            echo ""
            echo "Comparing images with ImageMagick..."
            compare -metric RMSE backend_test_osmesa.ppm backend_test_egl.ppm backend_test_diff.png 2>&1 | tee compare_result.txt
            echo "Difference image saved to backend_test_diff.png"
        else
            echo "ImageMagick not found, skipping detailed comparison"
        fi
    else
        echo "Error: Output files not found"
        ls -la *.ppm
    fi
else
    echo "Backend test failed with exit code $TEST_RESULT"
fi

exit $TEST_RESULT