# LVGL headless renderer image (lvgl-mcp).
# Prebuilds LVGL + harness; per-run only compiles the user's ui.c.
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc g++ make cmake git curl ca-certificates libc6-dev \
    && rm -rf /var/lib/apt/lists/*

ARG LVGL_TAG=v9.4.0
WORKDIR /opt

# --- LVGL static library (no examples/demos) ---
RUN git clone --depth 1 --branch ${LVGL_TAG} https://github.com/lvgl/lvgl.git /opt/lvgl
# LVGL's desktop CMake expects lv_conf.h at the LVGL project root.
COPY lv_conf.h /opt/lvgl/lv_conf.h
# Build target is x86_64; drop ARM (Helium/NEON) assembly blend sources that
# CMake would otherwise try to assemble. Safe because LV_USE_DRAW_SW_ASM=NONE.
RUN find /opt/lvgl/src -name '*.S' -delete
RUN cmake -S /opt/lvgl -B /opt/lvgl/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DLV_CONF_BUILD_DISABLE_EXAMPLES=1 \
        -DLV_CONF_BUILD_DISABLE_DEMOS=1 \
    && cmake --build /opt/lvgl/build -j"$(nproc)" \
    && find /opt/lvgl/build -name 'liblvgl.a' | head -1 > /opt/lvgl_lib_path \
    && echo "liblvgl.a -> $(cat /opt/lvgl_lib_path)" \
    && test -s /opt/lvgl_lib_path

# --- lodepng (single-file PNG encoder, compiled as C) ---
RUN curl -fsSL https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.cpp -o /opt/lodepng.c \
    && curl -fsSL https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.h -o /opt/lodepng.h \
    && gcc -O2 -c /opt/lodepng.c -o /opt/lodepng.o

# --- Renderer harness (create_ui resolved at run time from /work/ui.c) ---
COPY src/main.c /opt/renderer/main.c
RUN gcc -O2 -DLV_CONF_INCLUDE_SIMPLE -I/opt/lvgl -I/opt \
        -c /opt/renderer/main.c -o /opt/renderer/main.o

# --- shared fonts (e.g. Google Material Icons) linked into every render/sim ---
COPY fonts/material_icons.c /opt/renderer/material_icons.c
RUN gcc -O2 -DLV_CONF_INCLUDE_SIMPLE -I/opt/lvgl -I/opt \
        -c /opt/renderer/material_icons.c -o /opt/renderer/material_icons.o

COPY render.sh /opt/renderer/render.sh
RUN chmod +x /opt/renderer/render.sh

# --- interaction simulator harness (virtual touch + scripted actions) ---
COPY src/sim.c /opt/renderer/sim.c
RUN gcc -O2 -DLV_CONF_INCLUDE_SIMPLE -I/opt/lvgl -I/opt \
        -c /opt/renderer/sim.c -o /opt/renderer/sim.o
COPY simulate.sh /opt/renderer/simulate.sh
RUN chmod +x /opt/renderer/simulate.sh

WORKDIR /work
ENTRYPOINT ["/opt/renderer/render.sh"]
