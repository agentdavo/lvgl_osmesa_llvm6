#!/bin/bash
# Test script to verify LVGL display

echo "Testing LVGL display capabilities..."
echo ""

# Test with SDL backend
echo "1. Testing with SDL backend:"
export SDL_VIDEODRIVER=x11
timeout 3 build/src/lvgl_osmesa/lvgl_osmesa 2>&1 | head -20
echo "Exit code: $?"
echo ""

# Test with different SDL driver
echo "2. Testing with SDL software driver:"
export SDL_VIDEODRIVER=software  
timeout 3 build/src/lvgl_osmesa/lvgl_osmesa 2>&1 | head -20
echo "Exit code: $?"
echo ""

# Check if X11 is available
echo "3. Checking X11 availability:"
echo "DISPLAY=$DISPLAY"
xdpyinfo 2>&1 | head -5 || echo "xdpyinfo not available"
echo ""

# Check SDL availability
echo "4. Checking SDL2 availability:"
sdl2-config --version 2>&1 || echo "sdl2-config not found"
echo ""

# Try running dx8_cube with debug
echo "5. Running dx8_cube with debug environment:"
export DX8GL_LOG_LEVEL=DEBUG
export MESA_DEBUG=1
timeout 5 build/src/dx8_cube/dx8_cube 2>&1 | head -50
echo "Exit code: $?"