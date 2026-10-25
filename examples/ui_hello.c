/* Sample UI for the headless renderer.
 * Contract: define `void create_ui(lv_obj_t *parent)` using standard LVGL v9 API.
 * `parent` is the active screen. The same function compiles unchanged on the
 * M5Stack CoreS3 target. */
#include "lvgl.h"

void create_ui(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0E2438), 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Hello, CoreS3");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "LVGL headless render -> PNG");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x9FC7E8), 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 160, 48);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 20);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, LV_SYMBOL_OK "  Start");
    lv_obj_center(btn_label);

    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, 240, 16);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_bar_set_value(bar, 70, LV_ANIM_OFF);
}
