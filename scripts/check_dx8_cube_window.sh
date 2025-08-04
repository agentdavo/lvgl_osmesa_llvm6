#!/bin/bash
# Script to check if dx8_cube window is working

echo "Checking dx8_cube window functionality..."
echo ""

# Kill any existing dx8_cube processes
pkill -9 dx8_cube 2>/dev/null

# Set up environment
export LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu
export DX8GL_BACKEND=osmesa
export MESA_DEBUG=1

echo "Starting dx8_cube in background..."
cd build/src/dx8_cube
./dx8_cube > dx8_cube_output.log 2>&1 &
DX8_PID=$!

echo "PID: $DX8_PID"
echo "Waiting 3 seconds for initialization..."
sleep 3

# Check if process is still running
if kill -0 $DX8_PID 2>/dev/null; then
    echo "dx8_cube is running"
    
    # Check for window using xwininfo
    echo ""
    echo "Looking for windows with 'DirectX' in title:"
    xwininfo -root -tree 2>/dev/null | grep -i "directx" || echo "No DirectX windows found"
    
    echo ""
    echo "Looking for SDL windows:"
    xwininfo -root -tree 2>/dev/null | grep -i "sdl" || echo "No SDL windows found"
    
    echo ""
    echo "Last 50 lines of output:"
    tail -50 dx8_cube_output.log
    
    # Kill the process
    kill $DX8_PID 2>/dev/null
    wait $DX8_PID 2>/dev/null
else
    echo "dx8_cube crashed or exited"
    echo ""
    echo "Output log:"
    cat dx8_cube_output.log
fi

echo ""
echo "Checking for PPM files generated:"
ls -la *.ppm 2>/dev/null || echo "No PPM files found"

cd - >/dev/null