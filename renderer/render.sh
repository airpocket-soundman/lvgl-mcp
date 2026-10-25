#!/bin/sh
# Entrypoint: compile the user's ui.c together with the prebuilt harness + LVGL,
# then render to PNG. LVGL itself is prebuilt into the image, so only ui.c is
# compiled per run (a few seconds).
#
# Args: <ui.c> <out.png> [width] [height]
set -e

UI="${1:-/work/ui.c}"
OUT="${2:-/work/out.png}"
W="${3:-320}"
H="${4:-240}"

if [ ! -f "$UI" ]; then
    echo "ui source not found: $UI" >&2
    exit 1
fi

LVGL_LIB="$(cat /opt/lvgl_lib_path)"

# Compile + link (ui.c is source; main.o, lodepng.o, liblvgl.a are prebuilt)
gcc -O2 -DLV_CONF_INCLUDE_SIMPLE -I/opt/lvgl -I/opt \
    "$UI" /opt/renderer/main.o /opt/renderer/material_icons.o /opt/lodepng.o "$LVGL_LIB" \
    -lm -lpthread \
    -o /tmp/app

/tmp/app "$OUT" "$W" "$H"
