#!/usr/bin/env python3
import sys
from collections import defaultdict

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
    
    # Collect color statistics
    color_counts = defaultdict(int)
    bg_r, bg_g, bg_b = 64, 64, 128  # Clear color
    
    # Sample specific regions
    floor_y = int(height * 0.7)  # Lower part where floor should be
    cube_y = int(height * 0.4)   # Middle where cube should be
    
    floor_colors = defaultdict(int)
    cube_colors = defaultdict(int)
    
    for y in range(height):
        for x in range(width):
            r = ord(f.read(1))
            g = ord(f.read(1))
            b = ord(f.read(1))
            
            if r != bg_r or g != bg_g or b != bg_b:
                color = (r, g, b)
                color_counts[color] += 1
                
                # Sample floor region
                if y > floor_y:
                    floor_colors[color] += 1
                # Sample cube region  
                elif abs(y - cube_y) < 50:
                    cube_colors[color] += 1
    
    print(f'\nTotal unique colors: {len(color_counts)}')
    print(f'Total non-background pixels: {sum(color_counts.values())}')
    
    print('\nTop 10 most common colors:')
    for color, count in sorted(color_counts.items(), key=lambda x: x[1], reverse=True)[:10]:
        print(f'  RGB({color[0]:3}, {color[1]:3}, {color[2]:3}) : {count:5} pixels')
    
    print(f'\nFloor region colors (y > {floor_y}):')
    for color, count in sorted(floor_colors.items(), key=lambda x: x[1], reverse=True)[:5]:
        print(f'  RGB({color[0]:3}, {color[1]:3}, {color[2]:3}) : {count:5} pixels')
        
    print(f'\nCube region colors (around y={cube_y}):')
    for color, count in sorted(cube_colors.items(), key=lambda x: x[1], reverse=True)[:5]:
        print(f'  RGB({color[0]:3}, {color[1]:3}, {color[2]:3}) : {count:5} pixels')