#!/bin/bash

export LIBGL_ALWAYS_SOFTWARE=1
export DX8GL_BACKEND=egl
export LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu

echo "Testing EGL backend with LIBGL_ALWAYS_SOFTWARE=1"
timeout 5 build/src/dx8_cube/dx8_cube 2>&1