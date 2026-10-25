#!/usr/bin/env python3
"""MCP server for lvgl-mcp.

Exposes the headless LVGL renderer as MCP tools so an agent/LLM can write an
LVGL `create_ui()` function, render it, and *see* the resulting screen
(320x240 by default, e.g. an M5Stack CoreS3 display) as an image — iterating
until the UI matches the spec.

Transport: stdio. The server runs locally on your own machine and talks to
your local Docker daemon, where the `lvgl-renderer` image lives.
"""
import os
import pathlib
import shutil
import subprocess
import tempfile

from mcp.server.fastmcp import FastMCP, Image

RENDERER_IMAGE = os.environ.get("LVGL_RENDERER_IMAGE", "lvgl-renderer:v9")
WORK_ROOT = pathlib.Path(os.environ.get("LVGL_WORK_ROOT",
                                        os.path.expanduser("~/lvgl-work")))
RENDER_TIMEOUT = int(os.environ.get("LVGL_RENDER_TIMEOUT", "120"))

mcp = FastMCP("lvgl-ui-renderer")


def _render(ui_code: str, width: int, height: int):
    """Compile + render ui_code. Returns (ok, png_bytes, stdout, stderr)."""
    WORK_ROOT.mkdir(parents=True, exist_ok=True)
    job = tempfile.mkdtemp(prefix="job_", dir=str(WORK_ROOT))
    try:
        jp = pathlib.Path(job)
        (jp / "ui.c").write_text(ui_code, encoding="utf-8")
        out = jp / "out.png"
        cmd = [
            "docker", "run", "--rm", "-v", f"{job}:/work",
            RENDERER_IMAGE, "/work/ui.c", "/work/out.png", str(width), str(height),
        ]
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True,
                                  timeout=RENDER_TIMEOUT)
        except subprocess.TimeoutExpired:
            return False, None, "", f"render timed out after {RENDER_TIMEOUT}s"
        ok = proc.returncode == 0 and out.exists()
        png = out.read_bytes() if ok else None
        return ok, png, proc.stdout, proc.stderr
    finally:
        shutil.rmtree(job, ignore_errors=True)


@mcp.tool()
def render_lvgl_ui(ui_code: str, width: int = 320, height: int = 240):
    """Render an LVGL v9 UI and return a PNG screenshot of the screen.

    `ui_code` must be C that defines:  void create_ui(lv_obj_t *parent);
    `parent` is the active screen. Use standard LVGL v9 API (the same code can
    run unchanged on a real device). Default canvas 320x240 matches an
    M5Stack CoreS3 display; pass width/height for other panels.

    On success returns the rendered image. On compile/render failure returns the
    gcc/renderer errors as text so you can fix the code and try again.

    Call `lvgl_widget_cheatsheet()` for a quick API reference.
    """
    ok, png, out, err = _render(ui_code, width, height)
    if not ok:
        return ("LVGL build/render FAILED — fix the C and retry.\n"
                f"--- stderr ---\n{(err or '')[-6000:]}\n"
                f"--- stdout ---\n{(out or '')[-1500:]}")
    return Image(data=png, format="png")


def _snapshot_names(actions):
    names = []
    for line in actions:
        parts = line.strip().split()
        if len(parts) >= 2 and parts[0] == "snapshot":
            names.append(parts[1])
    return names


def _simulate(ui_code, actions, width, height):
    """Replay actions; returns (ok, [(name, png_bytes)], stdout, stderr)."""
    WORK_ROOT.mkdir(parents=True, exist_ok=True)
    job = tempfile.mkdtemp(prefix="sim_", dir=str(WORK_ROOT))
    try:
        jp = pathlib.Path(job)
        (jp / "ui.c").write_text(ui_code, encoding="utf-8")
        (jp / "actions.txt").write_text("\n".join(actions) + "\n", encoding="utf-8")
        cmd = [
            "docker", "run", "--rm", "--entrypoint", "/opt/renderer/simulate.sh",
            "-v", f"{job}:/work", RENDERER_IMAGE,
            "/work/ui.c", "/work/actions.txt", str(width), str(height),
        ]
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True,
                                  timeout=RENDER_TIMEOUT)
        except subprocess.TimeoutExpired:
            return False, [], "", f"simulation timed out after {RENDER_TIMEOUT}s"
        frames = []
        for name in _snapshot_names(actions):
            p = jp / f"{name}.png"
            if p.exists():
                frames.append((name, p.read_bytes()))
        ok = proc.returncode == 0
        return ok, frames, proc.stdout, proc.stderr
    finally:
        shutil.rmtree(job, ignore_errors=True)


@mcp.tool()
def simulate_lvgl_ui(ui_code: str, actions: list[str], width: int = 320, height: int = 240):
    """Replay scripted touch interactions on an LVGL UI and return a sequence of screenshots.

    Use this to verify interactive behaviour (taps, swipes, scrolling, gestures,
    button-driven state/screen changes) — things a single static render can't show.

    `ui_code`: C defining `void create_ui(lv_obj_t *parent)` (same as render_lvgl_ui).
    `actions`: list of mini-DSL lines, executed in order:
        snapshot NAME              capture the current screen (returned as an image)
        tap X Y                    press then release at (X,Y)
        press X Y / move X Y / release
        swipe X1 Y1 X2 Y2 [STEPS] [MS]   drag (default 12 steps over 300 ms)
        wait MS                    advance time
    Include `snapshot` lines wherever you want to see the screen. Example:
        ["snapshot 1_home", "tap 95 96", "snapshot 2_light_on",
         "press 198 175", "move 60 175", "release", "snapshot 3_dim",
         "tap 300 18", "snapshot 4_settings"]
    Returns each named snapshot, in order, as a labelled image.
    """
    ok, frames, out, err = _simulate(ui_code, actions, width, height)
    if not frames:
        return ("Simulation produced no snapshots.\n"
                f"--- stderr ---\n{(err or '')[-4000:]}\n"
                f"--- stdout ---\n{(out or '')[-1500:]}\n"
                "Hint: include 'snapshot NAME' lines in actions.")
    result = []
    for name, png in frames:
        result.append(f"frame: {name}")
        result.append(Image(data=png, format="png"))
    return result


@mcp.tool()
def lvgl_widget_cheatsheet() -> str:
    """Quick reference of common LVGL v9 widget/style calls for create_ui()."""
    return CHEATSHEET


CHEATSHEET = r"""LVGL v9 quick reference — write `void create_ui(lv_obj_t *parent)`.
Canvas is 320x240 (M5Stack CoreS3). #include "lvgl.h".

SCREEN / BG
  lv_obj_set_style_bg_color(parent, lv_color_hex(0x101820), 0);

LABEL
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, "Hello");                 // or lv_label_set_text_fmt(l, "%d%%", v)
  lv_obj_set_style_text_color(l, lv_color_white(), 0);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 10);

BUTTON
  lv_obj_t *b = lv_button_create(parent);
  lv_obj_set_size(b, 140, 44);
  lv_obj_align(b, LV_ALIGN_CENTER, 0, 0);
  lv_obj_t *bl = lv_label_create(b); lv_label_set_text(bl, LV_SYMBOL_OK " OK"); lv_obj_center(bl);

BAR / SLIDER / ARC / SWITCH / CHECKBOX
  lv_obj_t *bar = lv_bar_create(parent); lv_obj_set_size(bar,200,14); lv_bar_set_value(bar,70,LV_ANIM_OFF);
  lv_obj_t *sl  = lv_slider_create(parent); lv_slider_set_value(sl,40,LV_ANIM_OFF);
  lv_obj_t *arc = lv_arc_create(parent); lv_obj_set_size(arc,100,100); lv_arc_set_value(arc,60);
  lv_obj_t *sw  = lv_switch_create(parent); lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_t *cb  = lv_checkbox_create(parent); lv_checkbox_set_text(cb, "Enable");

LIST / DROPDOWN / ROLLER
  lv_obj_t *list = lv_list_create(parent); lv_obj_set_size(list,200,160);
  lv_list_add_button(list, LV_SYMBOL_FILE, "Item A");
  lv_obj_t *dd = lv_dropdown_create(parent); lv_dropdown_set_options(dd, "One\nTwo\nThree");

LAYOUT (flex)
  lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

STYLE (inline, on a part/state)
  lv_obj_set_style_radius(obj, 8, 0);
  lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_set_style_border_width(obj, 2, 0);
  lv_obj_set_style_pad_all(obj, 6, 0);

COLORS / FONTS / SYMBOLS
  lv_color_hex(0xRRGGBB), lv_color_white(), lv_palette_main(LV_PALETTE_RED/GREEN/BLUE/...)
  fonts: &lv_font_montserrat_{8,10,12,14,16,18,20,22,24,28,32,36,48}
  symbols: LV_SYMBOL_OK/CLOSE/WIFI/BATTERY_FULL/SETTINGS/HOME/PLAY/PAUSE/BELL/GPS/...

ALIGN anchors: LV_ALIGN_{TOP,BOTTOM}_{LEFT,MID,RIGHT}, LV_ALIGN_{LEFT,RIGHT}_MID, LV_ALIGN_CENTER
"""


if __name__ == "__main__":
    mcp.run()
