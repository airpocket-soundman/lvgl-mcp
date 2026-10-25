---
name: lvgl-mcp
description: Develop LVGL v9 device UIs with a visual feedback loop. Use the lvgl-mcp MCP server to render a C `create_ui()` function headlessly to a PNG, look at the rendered screen, fix the code, and replay scripted touch interactions (tap/swipe/drag) as screenshot sequences — without any display hardware or device flashing. Use whenever you are writing, reviewing, or debugging LVGL UI code and need to verify what it actually looks like or how it behaves.
---

# lvgl-mcp — write LVGL UI, render it, see it, fix it

lvgl-mcp is an MCP server that renders LVGL v9 UI code headlessly and returns
the result as images. Instead of guessing coordinates blind, you iterate:
**write → render → look → fix**, and for interactive UIs:
**write → simulate touch → look at the frames → fix**.

## When to use this skill

- You are creating or modifying an LVGL v9 screen (any embedded UI project).
- The user asks what a UI "looks like", wants a layout changed, or reports a
  visual/interaction bug.
- You need to verify button states, screen transitions, scrolling, sliders,
  gestures — anything a static code read cannot confirm.

Do **not** use it for non-LVGL UI frameworks, and do not use it to flash or
deploy to hardware (out of scope for v1).

## Prerequisites and setup

The MCP server runs locally over stdio and needs a local Docker daemon.

1. Clone the public repository (call it `$LVGL_MCP_HOME`):

   ```sh
   git clone https://github.com/airpocket-soundman/lvgl-mcp.git lvgl-mcp
   cd lvgl-mcp
   ```

2. Run the one-time setup — it builds the renderer Docker image
   (`lvgl-renderer:v9`) and installs the server's Python dependencies into a
   virtualenv:

   ```sh
   ./setup.sh
   ```

   Requirements: Docker (daemon running), Python 3.10+.

3. Register the server with the agent's MCP client. For `.mcp.json`-style
   configuration, copy the repo's `.mcp.json.example` and replace the
   placeholder with the absolute clone path:

   ```json
   {
     "mcpServers": {
       "lvgl-ui": {
         "command": "/ABSOLUTE/PATH/TO/lvgl-mcp/mcp_server/run.sh",
         "args": []
       }
     }
   }
   ```

4. Restart/reconnect the client so the `lvgl-ui` tools appear. If renders
   fail with Docker errors, re-check that the daemon is running and that
   `./setup.sh` completed.

Useful environment variables (set in the MCP server config `env` if needed):
`LVGL_RENDERER_IMAGE` (default `lvgl-renderer:v9`), `LVGL_WORK_ROOT`
(default `~/lvgl-work`), `LVGL_RENDER_TIMEOUT` (default `120` seconds).

## The create_ui() contract

Every tool call takes C code that defines exactly one entry point:

```c
#include "lvgl.h"

void create_ui(lv_obj_t *parent)
{
    /* build the whole screen as children of `parent` (the active screen) */
}
```

Rules:

- Use only standard LVGL v9 API. No `main()`, no display/RTOS setup, no
  hardware calls — the harness owns initialisation.
- The same function is meant to be dropped unchanged into a real device
  project later, so keep it portable: no host-only headers or globals that a
  firmware build could not provide.
- Before writing non-trivial code, call `lvgl_widget_cheatsheet()` for the
  API quick reference (widgets, flex layout, styles, fonts, symbols).

## Tool usage

### render_lvgl_ui(ui_code, width=320, height=240)

- Returns a PNG of the screen on success.
- On compile/render failure it returns the gcc/renderer output as text:
  read the error, fix the C, and call again. Do not ask the user to interpret
  compiler errors for you.
- After a successful render, actually inspect the image: check alignment,
  clipping, contrast, spacing, and whether every requested element is
  present and legible. Fix discrepancies and re-render until the image
  matches the request.

### simulate_lvgl_ui(ui_code, actions, width=320, height=240)

Replays scripted touch input and returns the screenshots taken at each
`snapshot` line, in order, as labelled images. Use it to verify behaviour a
single render cannot show: toggles, sliders, screen navigation, scrolling,
gestures.

Action DSL (one string per action, executed in order):

| Action | Meaning |
|---|---|
| `snapshot NAME` | capture the current screen as frame `NAME` |
| `tap X Y` | press then release at (X, Y) |
| `press X Y` | touch down at (X, Y) |
| `move X Y` | move the touch point to (X, Y) |
| `release` | touch up |
| `swipe X1 Y1 X2 Y2 [STEPS] [MS]` | drag from (X1,Y1) to (X2,Y2); default 12 steps over 300 ms |
| `wait MS` | let time pass (animations, timers) |

Always bracket the interesting moments with `snapshot` lines — a simulation
with no snapshots returns no images. Example:

```
["snapshot 1_home", "tap 95 96", "snapshot 2_light_on",
 "press 198 175", "move 60 175", "release", "snapshot 3_dim",
 "swipe 300 120 20 120", "snapshot 4_next_screen"]
```

Coordinates are pixels in the same canvas space as `width`/`height`.

## Layout guidance

- **Design responsively.** The canvas size is a per-call parameter (320×240
  by default, e.g. a small device panel; 1280×720 works too). Use flex
  layouts (`lv_obj_set_flex_flow`, `lv_obj_set_flex_align`) and percentage
  sizing (`lv_pct()`) instead of hard-coding absolute positions wherever
  possible, so the same `create_ui()` looks right at any resolution.
- Prefer anchoring (`lv_obj_align`) with offsets over raw `set_pos` for
  fixed elements you do place absolutely.
- Keep text inside its containers: set sizes, use `lv_label_set_long_mode`
  where truncation is acceptable, and verify by looking at the render.
- Use the built-in Montserrat fonts and `LV_SYMBOL_*` icons listed in the
  cheatsheet; a Material Icons font is linked into every render.

## Workflow summary

1. (If needed) `lvgl_widget_cheatsheet()`.
2. Write `create_ui()` for the requested screen.
3. `render_lvgl_ui(...)` → look at the PNG → fix → repeat until it matches.
4. If the UI is interactive, `simulate_lvgl_ui(...)` with snapshots around
   each interaction → look at the frames → fix → repeat.
5. Hand the final `create_ui()` code to the user / save it into their
   project. Remind them it drops into their firmware project unchanged;
   flashing the device is outside this tool's scope.
