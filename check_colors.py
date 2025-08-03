#!/usr/bin/env python3
import sys
from collections import Counter

def check_ppm_colors(filename):
    with open(filename, 'rb') as f:
        # Read header
        magic = f.readline().decode().strip()
        if magic != 'P6':
            print(f"Not a P6 PPM file: {magic}")
            return
        
        # Skip comments
        dims = f.readline().decode().strip()
        while dims.startswith('#'):
            dims = f.readline().decode().strip()
        
        width, height = map(int, dims.split())
        max_val = int(f.readline().decode().strip())
        
        # Read pixel data
        pixels = f.read()
        
    # Count unique colors
    colors = Counter()
    background = (64, 64, 128)  # Dark blue background
    
    for y in range(height):
        for x in range(width):
            idx = (y * width + x) * 3
            r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
            if (r, g, b) != background:
                colors[(r, g, b)] += 1
    
    print(f"Unique non-background colors: {len(colors)}")
    for color, count in colors.most_common(10):
        print(f"  RGB({color[0]:3}, {color[1]:3}, {color[2]:3}): {count:5} pixels")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: check_colors.py <ppm_file>")
        sys.exit(1)
    check_ppm_colors(sys.argv[1])