# lvgl-mcp MCP server

Exposes the headless LVGL renderer as MCP tools so an agent can write an LVGL
`create_ui()` function, render it, and **see** the resulting screen — iterating
until the UI matches the spec.

Everything runs **locally on your own machine**: the server uses the stdio
transport, is launched directly by your MCP client, and calls your local
Docker daemon, where the `lvgl-renderer:v9` image lives. No remote server,
SSH, or network service is involved.

## Tools

- `render_lvgl_ui(ui_code, width=320, height=240)` → renders the C `create_ui()`
  and returns a **PNG image**. On compile/render error it returns the
  gcc/renderer text so the agent can fix the code and retry.
- `simulate_lvgl_ui(ui_code, actions)` → replays scripted touch actions
  (tap/press/move/release/swipe/wait) via a virtual input device and returns a
  **sequence of screenshots**, so interactions (toggles, drags, gestures,
  screen navigation) can be verified headlessly. `actions` is a list of
  mini-DSL lines; include `snapshot NAME` lines where you want frames.
- `lvgl_widget_cheatsheet()` → quick LVGL v9 API reference for writing
  `create_ui()`.

## Setup

From the repository root, run once:

```sh
./setup.sh
```

That builds the renderer Docker image and creates `mcp_server/.venv` with the
Python dependencies. (`run.sh` also creates the venv on first launch if you
skip this step, but the Docker image must exist before the first render —
`setup.sh` handles both.)

## Wiring into an MCP client

Copy [`.mcp.json.example`](../.mcp.json.example) from the repository root to
`.mcp.json` in your project (or merge the entry into your client's MCP
config), replacing `/ABSOLUTE/PATH/TO/lvgl-mcp` with the path where you cloned
this repository:

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

- Approve the project MCP server in your client on first use (in Claude Code,
  run `/mcp`).
- After changing `server.py`, reconnect/restart the client so it picks up the
  new tool list.

## Verify the transport from a shell (optional)

This should print the three tool names:

```sh
printf '%s\n' \
  '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"probe","version":"0"}}}' \
  '{"jsonrpc":"2.0","method":"notifications/initialized"}' \
  '{"jsonrpc":"2.0","id":2,"method":"tools/list"}' \
| ./run.sh
```

## Troubleshooting

- **Docker errors / "image not found"**: run `./setup.sh` from the repository
  root, and make sure the Docker daemon is running and your user can run
  `docker run hello-world`.
- **MCP server shows Failed**: check that `run.sh` is executable
  (`chmod +x run.sh`) and that `python3` (3.10+) is on your `PATH`.
- **Slow first render**: the first call builds nothing, but Docker may still
  be starting; later renders take a few seconds because only `ui.c` is
  recompiled per run.

## Env vars

- `LVGL_RENDERER_IMAGE` (default `lvgl-renderer:v9`)
- `LVGL_WORK_ROOT` (default `~/lvgl-work`) — scratch directory for render jobs
- `LVGL_RENDER_TIMEOUT` seconds (default `120`)
