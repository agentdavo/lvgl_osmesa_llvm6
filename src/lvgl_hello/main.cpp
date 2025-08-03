#include "lvgl_platform.h"
#include <lvgl.h>

static void btn_event_cb(lv_event_t *e)
{
    lv_obj_t *lbl = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    lv_label_set_text(lbl, "Clicked");
}

int main() {
    lv_display_t *disp = LvglPlatform::create_window(800, 600, "sdl");
    if(!disp) return 1;
    lv_obj_t *hello = lv_label_create(lv_screen_active());
    lv_label_set_text(hello, "Hello world");
    lv_obj_align(hello, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_center(btn);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Press me");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, btn_label);
    while(true) {
        LvglPlatform::poll_events();
    }
    return 0;
}
