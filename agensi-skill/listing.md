# Agensi listing draft — lvgl-mcp

## Proposed title

**LVGL MCP — Let Your Agent See the UI It Writes (Headless Render + Touch Simulation)**

## Short description

Your agent writes LVGL v9 `create_ui()` C code; lvgl-mcp compiles and renders
it headlessly to a PNG, so the agent can actually *look* at the screen, fix
what's off, and iterate — no display, no device, no flashing. A touch
simulator replays taps, swipes and drags and returns screenshot sequences to
verify interactions. Runs 100% locally: a stdio MCP server driving your own
Docker daemon. Works with Claude Code, Cursor, Copilot and other MCP
clients.

## Price suggestion

- **$19 one-time** (low-priced launch, as decided by the author).
- Platform split on Agensi: the creator keeps **70%** (≈ $13.30 per sale);
  Agensi keeps 30%. No listing fee.
- Revisit after the first reviews: a subscription tier only makes sense once
  there is an update cadence (new LVGL versions, more harnesses) to justify
  it.

## What's included

- The `lvgl-mcp` repository:
  - `mcp_server/` — FastMCP stdio server with three tools:
    `render_lvgl_ui`, `simulate_lvgl_ui`, `lvgl_widget_cheatsheet`.
  - `renderer/` — Docker image (Debian + LVGL v9.4.0 prebuilt) with the
    headless renderer and touch-simulator harnesses; only your `ui.c` is
    recompiled per run, so iterations take seconds.
  - `setup.sh` + `.mcp.json.example` — one-command setup and client wiring.
  - `examples/` — a hello-world UI and simulator screenshot sequences.
- This SKILL.md, which teaches the buyer's agent when and how to drive the
  server (setup, the `create_ui()` contract, the touch-action DSL,
  responsive-layout guidance).
- Any canvas size (320×240 default — small-panel size — up to large panels
  such as 1280×720), Material Icons font included, gcc errors returned as
  readable text for the agent's fix loop.

## Requirements (state on the listing)

- Docker (local daemon), Python 3.10+, an MCP client.
- v1 scope: rendering + touch simulation only. Flashing real hardware is
  out of scope; the validated `create_ui()` code drops into the buyer's own
  firmware project unchanged.

## Submission checklist

- [x] License chosen: MIT (LICENSE + NOTICE added to the repo).
- [x] Replace `<public-repo-url>` placeholders (README + SKILL.md) with the
      real URL of the new public repository.
- [ ] Zip `agensi-skill/` (SKILL.md at the package root level expected by
      the marketplace) together with any reference files Agensi requires.
- [ ] Connect payout (Stripe or USDC) and complete KYC in Agensi's seller
      flow — this step only the author can do.
- [ ] Submit at https://www.agensi.io/sell and pass the security/AI/human
      review (typically 24–48 h per Agensi).
