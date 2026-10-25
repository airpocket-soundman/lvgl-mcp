#!/bin/sh
# Launcher for the lvgl-mcp MCP server (stdio, local).
# Your MCP client (Claude Code, Cursor, ...) starts this script; the server
# then runs on your own machine and calls your local Docker daemon.
#
# First run: if .venv does not exist yet, it is created and the Python
# dependencies are installed automatically (messages go to stderr, so the
# stdio MCP protocol on stdout stays clean). You can also run ../setup.sh
# ahead of time to do the same thing plus build the renderer image.
set -e
cd "$(dirname "$0")"

if [ ! -x ./.venv/bin/python ]; then
    echo "lvgl-mcp: creating .venv and installing dependencies (first run)..." >&2
    python3 -m venv .venv
    ./.venv/bin/pip install -q -r requirements.txt
fi

exec ./.venv/bin/python server.py
