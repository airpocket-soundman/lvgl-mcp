/* Headless LVGL interaction simulator (lvgl-mcp).
 *
 * Like main.c, but registers a virtual pointer/touch input device and replays a
 * scripted sequence of actions (tap / swipe / press / move / release / wait),
 * capturing a PNG snapshot whenever the script asks. This lets an agent verify
 * touch interactions (gestures, scrolling, button-driven state changes) headlessly.
 *
 * Usage: sim <actions.txt> [width] [height]
 *   ui.c (defining create_ui) is compiled/linked together with this file.
 *   Snapshots are written to the current directory (the mounted /work) as <name>.png.
 *
 * actions.txt mini-DSL (one per line):
 *   snapshot NAME              capture current screen to NAME.png
 *   tap X Y                    press at (X,Y) then release
 *   press X Y                  pointer down at (X,Y)
 *   move X Y                   move pointer (while down)
 *   release                    pointer up
 *   swipe X1 Y1 X2 Y2 [STEPS] [MS]   drag from start to end (default 12 steps, 300ms)
 *   wait MS                    advance time by MS
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lvgl.h"
#include "lodepng.h"

extern void create_ui(lv_obj_t *parent);

static int32_t      SCR_W = 320, SCR_H = 240;
static uint8_t     *g_fb = NULL;          /* XRGB8888 byte order: B,G,R,X */
static lv_display_t *g_disp = NULL;

/* virtual pointer state */
static int32_t g_px = 0, g_py = 0;
static bool    g_pressed = false;

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const int32_t aw = lv_area_get_width(area);
    const size_t row_bytes = (size_t)aw * 4;
    for (int32_t y = area->y1; y <= area->y2; y++) {
        memcpy(g_fb + ((size_t)y * SCR_W + area->x1) * 4, px_map, row_bytes);
        px_map += row_bytes;
    }
    lv_display_flush_ready(disp);
}

static void indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = g_px;
    data->point.y = g_py;
    data->state = g_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void pump_ms(int ms)
{
    int n = ms / 16;
    if (n < 1) n = 1;
    for (int i = 0; i < n; i++) {
        lv_tick_inc(16);
        lv_timer_handler();
    }
}

static int save_png(const char *name)
{
    lv_refr_now(g_disp);
    const size_t n = (size_t)SCR_W * SCR_H;
    uint8_t *rgba = (uint8_t *)malloc(n * 4);
    if (!rgba) return 1;
    for (size_t i = 0; i < n; i++) {
        rgba[i * 4 + 0] = g_fb[i * 4 + 2];
        rgba[i * 4 + 1] = g_fb[i * 4 + 1];
        rgba[i * 4 + 2] = g_fb[i * 4 + 0];
        rgba[i * 4 + 3] = 255;
    }
    char path[256];
    snprintf(path, sizeof(path), "%s.png", name);
    unsigned err = lodepng_encode32_file(path, rgba, (unsigned)SCR_W, (unsigned)SCR_H);
    free(rgba);
    if (err) { fprintf(stderr, "lodepng error %u: %s\n", err, lodepng_error_text(err)); return 2; }
    fprintf(stderr, "snapshot -> %s\n", path);
    return 0;
}

static void do_swipe(int x1, int y1, int x2, int y2, int steps, int ms)
{
    if (steps < 1) steps = 1;
    int per = ms / steps; if (per < 1) per = 1;
    g_px = x1; g_py = y1; g_pressed = true; pump_ms(per);
    for (int s = 1; s <= steps; s++) {
        g_px = x1 + (x2 - x1) * s / steps;
        g_py = y1 + (y2 - y1) * s / steps;
        pump_ms(per);
    }
    g_pressed = false;
    pump_ms(120);
}

int main(int argc, char **argv)
{
    const char *actions = (argc > 1) ? argv[1] : "actions.txt";
    if (argc > 2) SCR_W = atoi(argv[2]);
    if (argc > 3) SCR_H = atoi(argv[3]);

    lv_init();
    g_fb = (uint8_t *)calloc((size_t)SCR_W * SCR_H, 4);
    g_disp = lv_display_create(SCR_W, SCR_H);
    lv_display_set_color_format(g_disp, LV_COLOR_FORMAT_XRGB8888);
    void *draw_buf = malloc((size_t)SCR_W * SCR_H * 4);
    lv_display_set_buffers(g_disp, draw_buf, NULL, (size_t)SCR_W * SCR_H * 4,
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(g_disp, flush_cb);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read_cb);
    lv_indev_set_display(indev, g_disp);

    create_ui(lv_screen_active());
    pump_ms(320);   /* let layout/styles settle */

    FILE *f = fopen(actions, "r");
    if (!f) { fprintf(stderr, "cannot open actions: %s\n", actions); return 1; }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char name[256];
        int a, b, c, d, e, g;
        if (sscanf(line, " snapshot %255s", name) == 1) {
            save_png(name);
        } else if (sscanf(line, " tap %d %d", &a, &b) == 2) {
            g_px = a; g_py = b; g_pressed = true; pump_ms(120);
            g_pressed = false; pump_ms(120);
        } else if (sscanf(line, " press %d %d", &a, &b) == 2) {
            g_px = a; g_py = b; g_pressed = true; pump_ms(60);
        } else if (sscanf(line, " move %d %d", &a, &b) == 2) {
            g_px = a; g_py = b; pump_ms(40);
        } else if (strncmp(line, "release", 7) == 0) {
            g_pressed = false; pump_ms(120);
        } else if ((g = sscanf(line, " swipe %d %d %d %d %d %d", &a, &b, &c, &d, &e, &g)) >= 4) {
            int steps = (g >= 5) ? e : 12;
            int ms = (g >= 6) ? g : 300;
            do_swipe(a, b, c, d, steps, ms);
        } else if (sscanf(line, " wait %d", &a) == 1) {
            pump_ms(a);
        }
        /* unknown / blank lines ignored */
    }
    fclose(f);
    return 0;
}
