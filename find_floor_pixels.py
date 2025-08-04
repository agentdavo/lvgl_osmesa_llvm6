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
    
    # Find where textured pixels are
    bg_r, bg_g, bg_b = 64, 64, 128  # Clear color
    
    # Look for light colored pixels (texture colors)
    texture_pixels = []
    
    for y in range(height):
        for x in range(width):
            r = ord(f.read(1))
            g = ord(f.read(1))
            b = ord(f.read(1))
            
            # Look for light/beige colors typical of wall texture
            if r > 180 and g > 180 and b > 160 and b < 190:
                texture_pixels.append((x, y, r, g, b))
    
    print(f'\nFound {len(texture_pixels)} texture-like pixels')
    
    if texture_pixels:
        # Find bounding box
        min_x = min(p[0] for p in texture_pixels)
        max_x = max(p[0] for p in texture_pixels)
        min_y = min(p[1] for p in texture_pixels)
        max_y = max(p[1] for p in texture_pixels)
        
        print(f'Texture pixel bounding box: ({min_x},{min_y}) to ({max_x},{max_y})')
        print(f'Center: ({(min_x+max_x)//2}, {(min_y+max_y)//2})')
        
        # Sample some pixels
        print('\nSample texture pixels:')
        for i in range(min(10, len(texture_pixels))):
            x, y, r, g, b = texture_pixels[i]
            print(f'  ({x:3},{y:3}): RGB({r},{g},{b})')