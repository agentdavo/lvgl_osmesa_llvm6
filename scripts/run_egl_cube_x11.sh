#!/bin/bash
cd build/src/dx8_cube
export LIBGL_ALWAYS_SOFTWARE=1
export DX8GL_BACKEND=egl
exec ./dx8_cube