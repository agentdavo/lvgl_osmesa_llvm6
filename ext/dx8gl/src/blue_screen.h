#ifndef DX8GL_BLUE_SCREEN_H
#define DX8GL_BLUE_SCREEN_H

#include <cstdint>
#include <cstring>

namespace dx8gl {

class BlueScreen {
public:
    // Fill framebuffer with blue screen of death pattern
    static void fill_framebuffer(void* framebuffer, int width, int height, 
                                const char* error_message = nullptr) {
        if (!framebuffer) return;
        
        // Blue color (RGBA)
        const uint8_t blue_r = 0;
        const uint8_t blue_g = 0;
        const uint8_t blue_b = 170;
        const uint8_t blue_a = 255;
        
        // Fill with blue
        uint8_t* pixels = static_cast<uint8_t*>(framebuffer);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 4;
                pixels[idx + 0] = blue_r;
                pixels[idx + 1] = blue_g;
                pixels[idx + 2] = blue_b;
                pixels[idx + 3] = blue_a;
            }
        }
        
        // Add white box in center for error message
        if (error_message) {
            int box_width = width * 3 / 4;
            int box_height = height / 4;
            int box_x = (width - box_width) / 2;
            int box_y = (height - box_height) / 2;
            
            // Draw white box
            for (int y = box_y; y < box_y + box_height && y < height; y++) {
                for (int x = box_x; x < box_x + box_width && x < width; x++) {
                    int idx = (y * width + x) * 4;
                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = 255;
                    pixels[idx + 2] = 255;
                    pixels[idx + 3] = 255;
                }
            }
            
            // Draw black border
            for (int i = 0; i < 3; i++) {
                // Top and bottom borders
                for (int x = box_x; x < box_x + box_width && x < width; x++) {
                    if (box_y + i < height) {
                        int idx = ((box_y + i) * width + x) * 4;
                        pixels[idx + 0] = 0;
                        pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;
                    }
                    if (box_y + box_height - 1 - i >= 0 && box_y + box_height - 1 - i < height) {
                        int idx = ((box_y + box_height - 1 - i) * width + x) * 4;
                        pixels[idx + 0] = 0;
                        pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;
                    }
                }
                
                // Left and right borders
                for (int y = box_y; y < box_y + box_height && y < height; y++) {
                    if (box_x + i < width) {
                        int idx = (y * width + (box_x + i)) * 4;
                        pixels[idx + 0] = 0;
                        pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;
                    }
                    if (box_x + box_width - 1 - i >= 0 && box_x + box_width - 1 - i < width) {
                        int idx = (y * width + (box_x + box_width - 1 - i)) * 4;
                        pixels[idx + 0] = 0;
                        pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;
                    }
                }
            }
            
            // Simple text rendering - "DX8GL ERROR" in center
            const char* text = "DX8GL RENDER ERROR";
            int text_len = strlen(text);
            int char_width = 8;
            int char_height = 16;
            int text_x = box_x + (box_width - text_len * char_width) / 2;
            int text_y = box_y + (box_height - char_height) / 2;
            
            // Draw simple block letters
            for (int i = 0; i < text_len; i++) {
                draw_char(pixels, width, height, text_x + i * char_width, text_y, text[i]);
            }
        }
    }
    
private:
    // Very simple character drawing (just blocks for now)
    static void draw_char(uint8_t* pixels, int width, int height, int x, int y, char c) {
        // Skip spaces
        if (c == ' ') return;
        
        // Draw a simple 6x12 block for each character
        for (int dy = 2; dy < 14; dy++) {
            for (int dx = 1; dx < 7; dx++) {
                int px = x + dx;
                int py = y + dy;
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    int idx = (py * width + px) * 4;
                    pixels[idx + 0] = 0;  // Black text
                    pixels[idx + 1] = 0;
                    pixels[idx + 2] = 0;
                    pixels[idx + 3] = 255;
                }
            }
        }
    }
};

} // namespace dx8gl

#endif // DX8GL_BLUE_SCREEN_H