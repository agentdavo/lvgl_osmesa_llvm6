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
    
    # Create a simple ASCII visualization
    pixels = []
    for y in range(height):
        row = []
        for x in range(width):
            r = ord(f.read(1))
            g = ord(f.read(1))
            b = ord(f.read(1))
            row.append((r, g, b))
        pixels.append(row)
    
    # Sample every 20 pixels for a rough view
    step = 20
    print('\nRough visualization (T=texture, C=cube colors, .=background):')
    for y in range(0, height, step):
        line = ''
        for x in range(0, width, step):
            r, g, b = pixels[y][x]
            if r == 64 and g == 64 and b == 128:
                line += '.'
            elif r > 180 and g > 180 and b > 160:
                line += 'T'  # Texture
            elif r == 0 and b == 255:  # Blue
                line += 'B'
            elif g == 255 and r == 0:  # Green
                line += 'G'
            elif r == 255 and g == 0:  # Red
                line += 'R'
            elif r == 255 and g == 255:  # Yellow
                line += 'Y'
            elif r == 0 and g > 0:  # Cyan-ish
                line += 'C'
            else:
                line += '#'  # Other colors
        print(f'Y{y:03d}: {line}')