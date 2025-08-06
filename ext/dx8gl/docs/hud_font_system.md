# HUD Font System Documentation

## Overview

The HUD (Heads-Up Display) system in dx8gl provides an overlay for displaying real-time information including FPS, debug text, statistics, and control instructions. The font rendering system has been enhanced to support both built-in fonts and external font textures.

## Features

### Font Loading Capabilities
- **Built-in Font**: 8x8 bitmap font covering ASCII characters 32-126
- **External Font Loading**: Support for loading custom font textures from files
- **Supported Formats**: TGA, BMP, PNG (through D3DXCreateTextureFromFileEx)
- **Auto-detection**: Automatic font metrics detection based on texture dimensions

### Font Texture Formats

The system recognizes several standard font texture layouts:

| Texture Size | Character Size | Grid Layout | Description |
|-------------|----------------|-------------|-------------|
| 128x48      | 8x8            | 16x6        | Default built-in font |
| 128x128     | 8x8            | 16x16       | Full ASCII character set |
| 256x128     | 16x16          | 16x8        | High-resolution font |
| 512x512     | 32x32          | 16x16       | Ultra-high resolution font |

### Font Texture Requirements

Font textures should follow these guidelines:
1. **Layout**: Characters arranged in a grid, starting with ASCII 32 (space)
2. **Order**: Left-to-right, top-to-bottom arrangement
3. **Format**: Preferably ARGB8888 with alpha channel for transparency
4. **Background**: Transparent background (alpha = 0) recommended
5. **Character Color**: White characters (RGB = 255,255,255) for proper tinting

## Usage

### Loading a Custom Font

```cpp
// During HUD initialization
dx8gl::HUDSystem* hud = dx8gl::HUD::Get();

// Try to load custom font (returns true if successful)
if (hud->LoadFontTexture("assets/fonts/custom_font.tga")) {
    // Custom font loaded successfully
} else {
    // Falls back to built-in font
}
```

### Font Search Paths

The HUD system attempts to load fonts from these locations (in order):
1. `assets/fonts/hud_font.tga`
2. `assets/fonts/hud_font.bmp`
3. `assets/fonts/hud_font.png`
4. Built-in font (fallback)

### Creating Custom Font Textures

To create a custom font texture:

1. **Use a Font Editor**: Tools like BMFont, Hiero, or FontBuilder
2. **Export Settings**:
   - Character set: ASCII 32-126 minimum
   - Texture format: TGA or BMP with alpha channel
   - Power-of-two dimensions recommended
3. **Grid Arrangement**: Ensure characters are evenly spaced in a grid

### Example Font Creation with ImageMagick

```bash
# Create a font texture from system font
convert -font "Courier-New" -pointsize 16 \
        -background transparent -fill white \
        label:"ASCII characters here..." \
        -crop 16x16 +repage \
        -montage -geometry 16x16+0+0 -tile 16x8 \
        font_texture.png
```

## API Reference

### HUDSystem Class Extensions

```cpp
class HUDSystem {
public:
    // Load font texture from file
    bool LoadFontTexture(const std::string& filename);
    
    // Create font texture from file (internal)
    bool CreateFontTextureFromFile(const std::string& filename);
    
    // Font metrics (adjustable based on loaded font)
    int m_charWidth;      // Width of each character
    int m_charHeight;     // Height of each character
    int m_charSpacing;    // Spacing between characters
    int m_lineHeight;     // Height of each text line
    int m_charsPerRow;    // Characters per row in texture
    int m_charRows;       // Number of character rows
};
```

## Testing

The HUD font system can be tested using:

```bash
# Build the test
cmake --build build --target test_hud_font

# Run the test
./build/Release/test_hud_font
```

The test demonstrates:
- Loading external font textures
- Fallback to built-in font
- Dynamic font metrics adjustment
- HUD rendering with custom fonts

## Performance Considerations

- Font textures are loaded once during initialization
- Character rendering uses hardware-accelerated texture mapping
- Batch rendering minimizes draw calls
- Font texture remains in VRAM (D3DPOOL_MANAGED)

## Troubleshooting

### Font Not Loading
- Check file path and permissions
- Verify texture format is supported (TGA, BMP, PNG)
- Ensure texture dimensions are valid

### Characters Not Displaying Correctly
- Verify font texture layout matches expected grid
- Check character ordering (should start with ASCII 32)
- Ensure proper alpha channel for transparency

### Performance Issues
- Use smaller font textures when possible
- Reduce HUD update frequency if needed
- Consider disabling unused HUD elements

## Future Enhancements

Potential improvements for the font system:
- Unicode/UTF-8 support
- SDF (Signed Distance Field) fonts for scaling
- Font atlas generation at runtime
- Kerning and ligature support
- Multiple font support (different fonts for different HUD elements)