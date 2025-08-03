#ifndef LVGL_PLATFORM_H
#define LVGL_PLATFORM_H

#include <lvgl.h>

namespace LvglPlatform {
    // Create a window with the specified backend
    lv_display_t* create_window(int width, int height, const char* backend);
    
    // Poll events
    void poll_events();
}

#endif // LVGL_PLATFORM_H