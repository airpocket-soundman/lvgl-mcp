#!/bin/sh
# One-time local setup for lvgl-mcp:
#   1. builds the headless renderer Docker image (lvgl-renderer:v9)
#   2. creates the MCP server's Python virtualenv and installs dependencies
#
# Requirements: Docker (daemon running) and Python 3.10+.
set -e
cd "$(dirname "$0")"

IMAGE="${LVGL_RENDERER_IMAGE:-lvgl-renderer:v9}"

echo "==> Checking Docker..."
docker version >/dev/null 2>&1 || {
    echo "error: cannot talk to the Docker daemon. Install Docker and make" >&2
    echo "       sure it is running, then re-run ./setup.sh" >&2
    exit 1
}

echo "==> Building renderer image ($IMAGE) from renderer/ ..."
docker build -t "$IMAGE" renderer/

echo "==> Setting up the MCP server virtualenv..."
cd mcp_server
if [ ! -x .venv/bin/python ]; then
    python3 -m venv .venv
fi
./.venv/bin/pip install -q -r requirements.txt
chmod +x run.sh
cd ..

echo ""
echo "Done. Next steps:"
echo "  1. Copy .mcp.json.example to .mcp.json in your project and replace"
echo "     /ABSOLUTE/PATH/TO/lvgl-mcp with the path of this clone:"
echo "       $(pwd)"
echo "  2. Restart your MCP client (or reconnect) and approve the lvgl-ui server."
echo "  3. Ask your agent to render the example in examples/ui_hello.c."
