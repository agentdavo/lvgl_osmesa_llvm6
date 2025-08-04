#!/usr/bin/env python3
import sys

# Read PPM file
filename = sys.argv[1] if len(sys.argv) > 1 else 'dx8_cube_frame_00.ppm'
with open(filename, 'rb') as f:
    # Skip header
    line = f.readline()  # P6
    dims = f.readline().decode().strip()
    f.readline()  # 255
    
    # Read pixel data
    width, height = map(int, dims.split())
    print(f'Image size: {width}x{height}')
    
    # Count non-background pixels
    bg_r, bg_g, bg_b = 64, 64, 128  # Clear color
    non_bg = 0
    
    for y in range(height):
        for x in range(width):
            r = ord(f.read(1))
            g = ord(f.read(1))
            b = ord(f.read(1))
            if r != bg_r or g != bg_g or b != bg_b:
                non_bg += 1
    
    print(f'Non-background pixels: {non_bg}')