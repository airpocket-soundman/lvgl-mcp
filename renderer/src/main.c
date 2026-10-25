/* Headless LVGL renderer harness (lvgl-mcp).
 *
 * Renders a user-supplied `create_ui(lv_obj_t *parent)` into an in-RAM
 * framebuffer (no SDL / no display hardware) and writes a PNG.
 *
 * Usage: app <out.png> [width] [height]
 *
 * The user's UI code (ui.c) is compiled and linked together with this file;
 * it must define:  void create_ui(lv_obj_t *parent);
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lvgl.h"
#include "lodepng.h"

/* Provided by the user's ui.c */
extern void create_ui(lv_obj_t *parent);

static int32_t  SCR_W = 320;   /* M5Stack CoreS3 default */
static int32_t  SCR_H = 240;
static uint8_t *g_fb  = NULL;  /* full frame, LVGL XRGB8888 byte order: B,G,R,X */

/* Copy each flushed region into the full framebuffer. */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const int32_t aw = lv_area_get_width(area);
    const size_t  row_bytes = (size_t)aw * 4;
    for (int32_t y = area->y1; y <= area->y2; y++) {
        uint8_t *dst = g_fb + ((size_t)y * SCR_W + area->x1) * 4;
        memcpy(dst, px_map, row_bytes);
        px_map += row_bytes;
    }
    lv_display_flush_ready(disp);
}

int main(int argc, char **argv)
{
    const char *out = (argc > 1) ? argv[1] : "out.png";
    if (argc > 2) SCR_W = atoi(argv[2]);
    if (argc > 3) SCR_H = atoi(argv[3]);
    if (SCR_W <= 0 || SCR_H <= 0 || SCR_W > 4096 || SCR_H > 4096) {
        fprintf(stderr, "invalid size %dx%d\n", SCR_W, SCR_H);
        return 1;
    }

    lv_init();

    g_fb = (uint8_t *)calloc((size_t)SCR_W * SCR_H, 4);
    if (!g_fb) { fprintf(stderr, "OOM framebuffer\n"); return 1; }

    lv_display_t *disp = lv_display_create(SCR_W, SCR_H);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_XRGB8888);

    const size_t draw_buf_sz = (size_t)SCR_W * SCR_H * 4;
    void *draw_buf = malloc(draw_buf_sz);
    if (!draw_buf) { fprintf(stderr, "OOM draw buffer\n"); return 1; }
    lv_display_set_buffers(disp, draw_buf, NULL, draw_buf_sz, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, flush_cb);

    /* Build the user's UI on the active screen */
    create_ui(lv_screen_active());

    /* Drive LVGL so layout + styles + initial animations settle, then force render */
    for (int i = 0; i < 30; i++) {
        lv_tick_inc(16);
        lv_timer_handler();
    }
    lv_refr_now(disp);

    /* Convert XRGB8888 (B,G,R,X) -> RGBA for PNG */
    const size_t n = (size_t)SCR_W * SCR_H;
    uint8_t *rgba = (uint8_t *)malloc(n * 4);
    if (!rgba) { fprintf(stderr, "OOM rgba\n"); return 1; }
    for (size_t i = 0; i < n; i++) {
        rgba[i * 4 + 0] = g_fb[i * 4 + 2]; /* R */
        rgba[i * 4 + 1] = g_fb[i * 4 + 1]; /* G */
        rgba[i * 4 + 2] = g_fb[i * 4 + 0]; /* B */
        rgba[i * 4 + 3] = 255;             /* A */
    }

    unsigned err = lodepng_encode32_file(out, rgba, (unsigned)SCR_W, (unsigned)SCR_H);
    if (err) {
        fprintf(stderr, "lodepng error %u: %s\n", err, lodepng_error_text(err));
        return 2;
    }
    fprintf(stderr, "rendered %dx%d -> %s\n", SCR_W, SCR_H, out);
    return 0;
}
