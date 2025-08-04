#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "lvgl/lvgl.h"

#define CANVAS_WIDTH 400
#define CANVAS_HEIGHT 400

// Function to show loading screen
static void show_loading_screen(uint8_t* canvas_buf, uint32_t start_time) {
    if (!canvas_buf) return;
    
    uint8_t* dst = canvas_buf;
    
    // Fill with dark blue background
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            int dst_idx = (y * CANVAS_WIDTH + x) * 4;
            dst[dst_idx + 0] = 80;   // B
            dst[dst_idx + 1] = 40;   // G
            dst[dst_idx + 2] = 20;   // R
            dst[dst_idx + 3] = 0xFF; // X
        }
    }
    
    // Draw "LOADING..." text in center (simple pixel art)
    const char* text = "LOADING...";
    int text_len = strlen(text);
    int char_width = 8;
    int char_height = 16;
    int text_x = (CANVAS_WIDTH - text_len * char_width) / 2;
    int text_y = (CANVAS_HEIGHT - char_height) / 2;
    
    // Simple white rectangle for each character position
    for (int i = 0; i < text_len; i++) {
        int cx = text_x + i * char_width;
        // Draw a simple white block for each character
        for (int y = 0; y < char_height - 2; y++) {
            for (int x = 0; x < char_width - 2; x++) {
                int px = cx + x;
                int py = text_y + y;
                if (px >= 0 && px < CANVAS_WIDTH && py >= 0 && py < CANVAS_HEIGHT) {
                    int dst_idx = (py * CANVAS_WIDTH + px) * 4;
                    dst[dst_idx + 0] = 255;  // B
                    dst[dst_idx + 1] = 255;  // G
                    dst[dst_idx + 2] = 255;  // R
                    dst[dst_idx + 3] = 0xFF; // X
                }
            }
        }
    }
    
    // Draw a progress bar below the text
    int bar_y = text_y + char_height + 10;
    int bar_width = 200;
    int bar_height = 10;
    int bar_x = (CANVAS_WIDTH - bar_width) / 2;
    
    // Calculate progress (0-100%)
    uint32_t elapsed = lv_tick_get() - start_time;
    int progress = (elapsed * 100) / 1000;  // 1 second = 100%
    if (progress > 100) progress = 100;
    
    // Draw progress bar outline
    for (int x = 0; x < bar_width; x++) {
        for (int y = 0; y < bar_height; y++) {
            int px = bar_x + x;
            int py = bar_y + y;
            if (px >= 0 && px < CANVAS_WIDTH && py >= 0 && py < CANVAS_HEIGHT) {
                int dst_idx = (py * CANVAS_WIDTH + px) * 4;
                // Border or fill
                if (y == 0 || y == bar_height - 1 || x == 0 || x == bar_width - 1) {
                    // White border
                    dst[dst_idx + 0] = 255;  // B
                    dst[dst_idx + 1] = 255;  // G
                    dst[dst_idx + 2] = 255;  // R
                } else if (x < (bar_width - 2) * progress / 100) {
                    // Green fill for progress
                    dst[dst_idx + 0] = 0;    // B
                    dst[dst_idx + 1] = 255;  // G
                    dst[dst_idx + 2] = 0;    // R
                }
                dst[dst_idx + 3] = 0xFF; // X
            }
        }
    }
}

// Save canvas to PPM file
static void save_ppm(const char* filename, uint8_t* canvas_buf) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return;
    
    fprintf(fp, "P6\n%d %d\n255\n", CANVAS_WIDTH, CANVAS_HEIGHT);
    
    // Convert BGRX to RGB
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            int idx = (y * CANVAS_WIDTH + x) * 4;
            uint8_t r = canvas_buf[idx + 2];
            uint8_t g = canvas_buf[idx + 1];
            uint8_t b = canvas_buf[idx + 0];
            fwrite(&r, 1, 1, fp);
            fwrite(&g, 1, 1, fp);
            fwrite(&b, 1, 1, fp);
        }
    }
    
    fclose(fp);
}

int main() {
    // Initialize LVGL tick
    lv_init();
    
    // Allocate canvas buffer
    uint8_t* canvas_buf = (uint8_t*)malloc(CANVAS_WIDTH * CANVAS_HEIGHT * 4);
    if (!canvas_buf) {
        std::cerr << "Failed to allocate canvas buffer" << std::endl;
        return 1;
    }
    
    // Test loading screen at different progress levels
    uint32_t start_time = lv_tick_get();
    
    // 0% progress
    show_loading_screen(canvas_buf, start_time);
    save_ppm("loading_screen_0.ppm", canvas_buf);
    std::cout << "Saved loading_screen_0.ppm (0% progress)" << std::endl;
    
    // Simulate 50% progress by adjusting calculation
    uint32_t fake_time_50 = start_time - 500;  // Make it seem like 500ms passed
    show_loading_screen(canvas_buf, fake_time_50);
    save_ppm("loading_screen_50.ppm", canvas_buf);
    std::cout << "Saved loading_screen_50.ppm (50% progress)" << std::endl;
    
    // Simulate 100% progress
    uint32_t fake_time_100 = start_time - 1000;  // Make it seem like 1000ms passed
    show_loading_screen(canvas_buf, fake_time_100);
    save_ppm("loading_screen_100.ppm", canvas_buf);
    std::cout << "Saved loading_screen_100.ppm (100% progress)" << std::endl;
    
    free(canvas_buf);
    return 0;
}