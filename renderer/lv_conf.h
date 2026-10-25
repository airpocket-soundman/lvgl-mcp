/* Minimal LVGL v9 configuration for headless framebuffer rendering (lvgl-mcp).
 * Placed at /opt/lv_conf.h in the Docker image and included via LV_CONF_INCLUDE_SIMPLE.
 * Widgets/themes default to ON in v9, so we only override what we care about. */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* 32-bit color: framebuffer is XRGB8888 -> trivial PNG export */
#define LV_COLOR_DEPTH 32

/* Use the C standard library */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

/* Software renderer */
#define LV_USE_DRAW_SW 1
/* No CPU-specific assembly blending (build target is x86_64, not ARM Helium/NEON) */
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE

#define LV_DEF_REFR_PERIOD 16

/* Logging: surface warnings/errors to stderr for build/render debugging */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* Fonts: a broad range of Montserrat sizes for flexible UI authoring */
#define LV_FONT_MONTSERRAT_8  1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Default theme */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 0

#endif /* LV_CONF_H */
