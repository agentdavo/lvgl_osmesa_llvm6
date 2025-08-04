#include "lvgl_platform.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <cstdlib>

#define CANVAS_WIDTH 400
#define CANVAS_HEIGHT 400
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static lv_obj_t* g_canvas = nullptr;
static lv_color_t* g_canvas_buf = nullptr;
static int g_frame_count = 0;

static void update_canvas(lv_timer_t* timer) {
    (void)timer;
    
    if (!g_canvas_buf) return;
    
    // Create a simple animated pattern
    uint8_t* dst = (uint8_t*)g_canvas_buf;
    
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            int dst_idx = (y * CANVAS_WIDTH + x) * 4;
            
            // Create animated gradient
            int r = (x + g_frame_count) % 256;
            int g = (y + g_frame_count) % 256;
            int b = ((x + y) / 2 + g_frame_count) % 256;
            
            // LVGL uses BGRX format
            dst[dst_idx + 0] = b;    // B
            dst[dst_idx + 1] = g;    // G
            dst[dst_idx + 2] = r;    // R
            dst[dst_idx + 3] = 0xFF; // X
        }
    }
    
    lv_obj_invalidate(g_canvas);
    g_frame_count++;
}

int main() {
    printf("Testing LVGL canvas rendering...\n");
    
    // Create LVGL window
    lv_display_t* display = LvglPlatform::create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "sdl");
    if (!display) {
        printf("Failed to create LVGL display\n");
        return 1;
    }
    
    // Allocate canvas buffer
    g_canvas_buf = (lv_color_t*)malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t));
    if (!g_canvas_buf) {
        printf("Failed to allocate canvas buffer\n");
        return 1;
    }
    
    // Get the active screen
    lv_obj_t* screen = lv_display_get_screen_active(display);
    
    // Create title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "LVGL Canvas Test");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create canvas
    g_canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(g_canvas, g_canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_XRGB8888);
    lv_obj_center(g_canvas);
    
    // Add border
    lv_obj_set_style_border_width(g_canvas, 2, 0);
    lv_obj_set_style_border_color(g_canvas, lv_color_hex(0x4080ff), 0);
    
    // Create timer to update canvas (30 FPS)
    lv_timer_create(update_canvas, 33, nullptr);
    
    printf("Running for 5 seconds...\n");
    
    // Run for 5 seconds
    for (int i = 0; i < 500; i++) {
        LvglPlatform::poll_events();
        lv_timer_handler();
        usleep(10000); // 10ms
    }
    
    printf("Test completed successfully\n");
    free(g_canvas_buf);
    return 0;
}