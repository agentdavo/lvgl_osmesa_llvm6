#include "lvgl_platform.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#ifdef LV_USE_SDL
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"
#include <SDL2/SDL.h>
#endif

#ifdef LV_USE_X11
#include "src/drivers/x11/lv_x11.h"
#endif

namespace LvglPlatform {

lv_display_t* create_window(int width, int height, const char* backend) {
    lv_init();
    
    // If no specific backend requested, try to auto-detect
    if (backend == nullptr || strlen(backend) == 0) {
        backend = "auto";
    }
    
    // Try backends in order of preference
    bool auto_detect = (strcmp(backend, "auto") == 0);
    
#ifdef LV_USE_X11
    if (auto_detect || strcmp(backend, "x11") == 0) {
        // Check if X11 display is available
        if (getenv("DISPLAY") != nullptr) {
            lv_display_t* disp = lv_x11_window_create("DirectX 8 Demo", width, height);
            if (disp) {
                // Create input devices
                lv_x11_inputs_create(disp, nullptr);
                if (auto_detect) {
                    printf("Using X11 backend\n");
                }
                return disp;
            } else if (!auto_detect) {
                printf("Failed to create X11 window\n");
                return nullptr;
            }
        } else if (!auto_detect) {
            printf("X11 requested but DISPLAY not set\n");
            return nullptr;
        }
    }
#endif
    
#ifdef LV_USE_SDL
    if (auto_detect || strcmp(backend, "sdl") == 0) {
        lv_display_t* disp = lv_sdl_window_create(width, height);
        if (disp) {
            lv_sdl_mouse_create();
            lv_sdl_keyboard_create();
            if (auto_detect) {
                printf("Using SDL backend\n");
            }
            return disp;
        } else if (!auto_detect) {
            printf("Failed to create SDL window\n");
            return nullptr;
        }
    }
#endif
    
#ifdef LV_USE_WAYLAND
    if (auto_detect || strcmp(backend, "wayland") == 0) {
        // Check if Wayland is available
        if (getenv("WAYLAND_DISPLAY") != nullptr) {
            // Wayland display creation would go here
            printf("Wayland backend not implemented yet\n");
            if (!auto_detect) {
                return nullptr;
            }
        } else if (!auto_detect) {
            printf("Wayland requested but WAYLAND_DISPLAY not set\n");
            return nullptr;
        }
    }
#endif
    
#ifdef LV_USE_LINUX_DRM
    if (auto_detect || strcmp(backend, "drm") == 0) {
        // DRM display creation would go here
        printf("DRM backend not implemented yet\n");
        if (!auto_detect) {
            return nullptr;
        }
    }
#endif
    
#ifdef LV_USE_LINUX_FBDEV
    if (auto_detect || strcmp(backend, "fbdev") == 0) {
        // Framebuffer display creation would go here
        printf("Framebuffer backend not implemented yet\n");
        if (!auto_detect) {
            return nullptr;
        }
    }
#endif
    
    if (auto_detect) {
        printf("No suitable backend found\n");
    } else {
        printf("Unsupported backend: %s\n", backend);
    }
    return nullptr;
}

void poll_events() {
    lv_timer_handler();
#ifdef LV_USE_SDL
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // SDL event handling is done by LVGL SDL driver
    }
#endif
}

} // namespace LvglPlatform