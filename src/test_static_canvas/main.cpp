#include "lvgl_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CANVAS_WIDTH 400
#define CANVAS_HEIGHT 400
#define WINDOW_WIDTH 800  
#define WINDOW_HEIGHT 600

int main() {
    printf("Testing static LVGL canvas...\n");
    
    // Create LVGL window
    lv_display_t* display = LvglPlatform::create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "sdl");
    if (!display) {
        printf("Failed to create LVGL display\n");
        return 1;
    }
    
    // Get the active screen
    lv_obj_t* screen = lv_display_get_screen_active(display);
    
    // Create title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Static Canvas Test - Red Square");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create canvas
    lv_obj_t* canvas = lv_canvas_create(screen);
    
    // Allocate buffer
    static lv_color_t canvas_buf[CANVAS_WIDTH * CANVAS_HEIGHT];
    lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_XRGB8888);
    lv_obj_center(canvas);
    
    // Fill with red color
    lv_canvas_fill_bg(canvas, lv_color_hex(0xFF0000), LV_OPA_COVER);
    
    // Draw a white rectangle in the center
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0xFFFFFF);
    rect_dsc.bg_opa = LV_OPA_COVER;
    
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    
    lv_area_t area;
    area.x1 = 150;
    area.y1 = 150;
    area.x2 = 250;
    area.y2 = 250;
    
    lv_draw_rect(&layer, &rect_dsc, &area);
    lv_canvas_finish_layer(canvas, &layer);
    
    // Add border
    lv_obj_set_style_border_width(canvas, 2, 0);
    lv_obj_set_style_border_color(canvas, lv_color_hex(0x4080ff), 0);
    
    printf("Canvas created. Running for 3 seconds...\n");
    
    // Run for 3 seconds
    for (int i = 0; i < 300; i++) {
        LvglPlatform::poll_events();
        lv_timer_handler();
        usleep(10000); // 10ms
    }
    
    printf("Test completed successfully\n");
    return 0;
}