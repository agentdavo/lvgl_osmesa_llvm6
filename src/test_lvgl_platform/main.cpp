#include "lvgl_platform.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Testing LVGL platform initialization...\n");
    
    // Create a simple LVGL window
    lv_display_t* display = LvglPlatform::create_window(800, 600, "sdl");
    if (!display) {
        printf("Failed to create LVGL display\n");
        return 1;
    }
    
    printf("LVGL display created successfully\n");
    
    // Get the active screen
    lv_obj_t* screen = lv_display_get_screen_active(display);
    
    // Create a label on the screen
    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, "LVGL Platform Test\nWindow is working!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    
    // Create a button
    lv_obj_t* btn = lv_button_create(screen);
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 60);
    
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Test Button");
    lv_obj_center(btn_label);
    
    // Set a background color
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), LV_PART_MAIN);
    
    printf("Running LVGL event loop for 5 seconds...\n");
    
    // Run for 5 seconds
    for (int i = 0; i < 500; i++) {
        LvglPlatform::poll_events();
        usleep(10000); // 10ms
    }
    
    printf("Test completed successfully\n");
    return 0;
}