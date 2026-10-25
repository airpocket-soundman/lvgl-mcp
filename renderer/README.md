# LVGL headless renderer

Compiles a user-supplied `create_ui(lv_obj_t *parent)` against LVGL v9 and
renders it to a PNG **without any display hardware** (in-RAM framebuffer →
`lodepng`). It runs locally inside Docker and is normally driven by the MCP
server in [`../mcp_server/`](../mcp_server/), so an LLM agent can see the
rendered UI, but it can also be used standalone from the command line.

- Default canvas: **320×240** (the size of an M5Stack CoreS3 / ILI9342C panel);
  any width/height can be passed per render.
- Color: XRGB8888 internally → RGBA PNG.
- The `create_ui()` you validate here is plain LVGL v9 C and can be reused
  unchanged in a device project of your own.

## Build

```sh
# from this directory (renderer/)
docker build -t lvgl-renderer:v9 .
```

## Render one UI

```sh
# ui.c must define: void create_ui(lv_obj_t *parent);
docker run --rm -v "$PWD":/work lvgl-renderer:v9 /work/ui.c /work/out.png 320 240
```

`out.png` is written next to `ui.c`. Compile errors from `ui.c` go to stderr and
the container exits non-zero.

## Simulate touch input

```sh
# actions.txt holds one action per line (snapshot / tap / press / move /
# release / swipe / wait — see the repository README for the full DSL)
docker run --rm --entrypoint /opt/renderer/simulate.sh \
    -v "$PWD":/work lvgl-renderer:v9 /work/ui.c /work/actions.txt 320 240
```

Snapshot PNGs are written into the mounted directory.

## How it fits together

```
ui.c  ──►  render.sh  ──►  gcc (ui.c + main.o + lodepng.o + liblvgl.a)  ──►  /tmp/app  ──►  out.png
```

LVGL and the harness are prebuilt into the image; each run only recompiles
`ui.c`, so iterations take a few seconds.
