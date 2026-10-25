#!/bin/sh
# Entrypoint: compile ui.c with the interaction simulator harness, then replay an
# actions script, writing snapshot PNGs into /work.
#
# Args: <ui.c> <actions.txt> [width] [height]
set -e

UI="${1:-/work/ui.c}"
ACTIONS="${2:-/work/actions.txt}"
W="${3:-320}"
H="${4:-240}"

[ -f "$UI" ]      || { echo "ui source not found: $UI" >&2; exit 1; }
[ -f "$ACTIONS" ] || { echo "actions not found: $ACTIONS" >&2; exit 1; }

LVGL_LIB="$(cat /opt/lvgl_lib_path)"

gcc -O2 -DLV_CONF_INCLUDE_SIMPLE -I/opt/lvgl -I/opt \
    "$UI" /opt/renderer/sim.o /opt/renderer/material_icons.o /opt/lodepng.o "$LVGL_LIB" \
    -lm -lpthread \
    -o /tmp/simapp

cd /work
/tmp/simapp "$ACTIONS" "$W" "$H"
