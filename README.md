# lvgl-mcp — Let your AI agent *see* the LVGL UI it writes

An MCP server that closes the feedback loop for LVGL UI development:

1. Your AI agent writes a plain LVGL v9 `create_ui()` function in C.
2. lvgl-mcp compiles it and renders it **headlessly to a PNG** — no display,
   no device, no flashing.
3. The agent actually **looks at the image**, spots what's off, and fixes
   the code — iterating until the screen matches the spec.
4. When layout alone is not enough, the agent can replay scripted **touch
   interactions** (taps, swipes, drags) and inspect a sequence of
   screenshots to verify behaviour.

Everything runs locally on your own machine: the MCP server talks to your
local Docker daemon, where a prebuilt LVGL renderer image does the compiling
and drawing. Compile errors are returned to the agent as text, rendered
screens as images — those two feedback channels are the whole loop.

```
agent ── render_lvgl_ui(code) / simulate_lvgl_ui(code, actions) ──► MCP server (local, stdio)
  ▲                                                                          │
  └──────────── PNG image / gcc errors ◀────────────── Docker: build → headless render
```

## Requirements

- **Docker** (daemon running; the renderer image is built from `renderer/`)
- **Python 3.10+** (for the MCP server; a virtualenv is created for you)
- An **MCP client** such as Claude Code, Cursor, or any other client that
  can launch a stdio MCP server
- macOS, Linux, or Windows (WSL2 recommended on Windows)

## Quickstart

```sh
# 1. Clone
git clone <this-repo-url> lvgl-mcp
cd lvgl-mcp

# 2. One-time setup: builds the renderer Docker image (lvgl-renderer:v9)
#    and installs the MCP server's Python dependencies into a venv
./setup.sh
```

(Prefer Compose for the image? `docker compose build renderer` does the same
build — see `docker-compose.yml`. The MCP server itself is not run via
Compose.)

```sh
# 3. Wire the server into your project: copy the example config into your
#    project as .mcp.json and replace the placeholder with the absolute
#    path of your clone
cp .mcp.json.example /path/to/your/project/.mcp.json
#    then edit /path/to/your/project/.mcp.json:
#    "/ABSOLUTE/PATH/TO/lvgl-mcp" -> e.g. "/home/you/lvgl-mcp"
```

4. Restart (or reconnect) your MCP client and approve the `lvgl-ui` server.
   In Claude Code, run `/mcp`.
5. Ask your agent to render [`examples/ui_hello.c`](examples/ui_hello.c) —
   or just describe a screen and let it write, render, look, and iterate.

## Tools

### `render_lvgl_ui(ui_code, width=320, height=240)`

Renders one screen and returns it as a PNG image. `ui_code` is C defining
`void create_ui(lv_obj_t *parent)`. On failure it returns the gcc/renderer
output as text so the agent can fix and retry.

### `simulate_lvgl_ui(ui_code, actions, width=320, height=240)`

Replays scripted touch actions and returns the screenshots captured at each
`snapshot` line, in order, as labelled images. Action DSL (one per line):

| Action | Meaning |
|---|---|
| `snapshot NAME` | capture the current screen as `NAME` |
| `tap X Y` | press then release at (X, Y) |
| `press X Y` / `move X Y` / `release` | build up drags and gestures |
| `swipe X1 Y1 X2 Y2 [STEPS] [MS]` | drag (default 12 steps over 300 ms) |
| `wait MS` | advance time |

Example:

```
["snapshot 1_home", "tap 95 96", "snapshot 2_light_on",
 "press 198 175", "move 60 175", "release", "snapshot 3_dim"]
```

### `lvgl_widget_cheatsheet()`

Returns a compact LVGL v9 API reference (labels, buttons, sliders, arcs,
flex layout, styles, fonts, symbols) that the agent can consult before
writing `create_ui()`.

## How it works

- **Renderer image** (`renderer/`): Debian + LVGL v9.4.0 prebuilt as a static
  library, plus two small harnesses — a static renderer (`src/main.c`) and an
  interaction simulator (`src/sim.c`) with a virtual touch input device.
  Per run, only your `ui.c` is compiled and linked against the prebuilt
  library, so an iteration takes a few seconds. Frames are drawn into an
  in-RAM framebuffer and encoded to PNG with lodepng. A Material Icons font
  is linked into every render.
- **MCP server** (`mcp_server/server.py`): a FastMCP stdio server. For each
  call it writes the UI code (and action script) into a temporary job
  directory, runs the renderer image with that directory mounted, and returns
  the PNG(s) or the compiler output. Job directories are cleaned up
  automatically.
- **Resolution is per-call**: `width`/`height` default to 320×240 (an
  M5Stack CoreS3-sized panel), but a 1280×720 panel works just as well —
  write layouts responsively with flex and `lv_pct()` so the same
  `create_ui()` adapts.

### Configuration (environment variables)

| Variable | Default | Meaning |
|---|---|---|
| `LVGL_RENDERER_IMAGE` | `lvgl-renderer:v9` | renderer Docker image to run |
| `LVGL_WORK_ROOT` | `~/lvgl-work` | scratch directory for render jobs |
| `LVGL_RENDER_TIMEOUT` | `120` | per-render timeout, seconds |

## Examples

- [`examples/ui_hello.c`](examples/ui_hello.c) — a minimal screen (title,
  button, progress bar) to smoke-test your setup.
- [`examples/sim3/`](examples/sim3/) — screenshots captured by the touch
  simulator from a two-screen demo UI (controller ⇄ app launcher, with
  scrolling and swipe navigation), showing the kind of frame sequences
  `simulate_lvgl_ui` returns.

## Scope of v1

**In scope:** headless rendering and touch simulation of LVGL v9
`create_ui()` code, driven by an AI agent through MCP.

**Out of scope (for now):** flashing or deploying to real hardware. The
`create_ui()` you validate here is plain LVGL v9 C and can be dropped into
your own device project unchanged — but wiring it into a firmware build and
flashing a device is your project's job, not this tool's.

## Repository layout

| Path | Contents |
|---|---|
| `mcp_server/` | the FastMCP stdio server (`server.py`), launcher, deps |
| `renderer/` | Dockerfile and harnesses for the headless renderer image |
| `examples/` | sample UI code and simulator screenshots |
| `agensi-skill/` | the packaged agent skill (SKILL.md) and listing draft |
| `setup.sh` | one-time local setup (image build + Python venv) |
| `.mcp.json.example` | MCP client config template |

## Origin

lvgl-mcp grew out of a personal project in which an AI agent developed the
UI for a small LVGL device this exact way — write, render, look, fix — with
the loop served from a dedicated home server over SSH. This repository is
the generalized version of that workflow: same renderer, same tools, same
feedback loop, but packaged to run entirely on your own machine.

## License — TODO

> **TODO (author):** choose a license for this project before publishing
> (e.g. MIT / Apache-2.0) and add it as a `LICENSE` file here. No license
> file is included yet, so no usage rights are granted by this repository
> beyond what your local law provides.
