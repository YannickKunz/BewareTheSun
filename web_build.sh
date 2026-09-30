#!/bin/sh
# Rebuild the Godot game and replace the tracked static Netlify publish directory.
set -eu
cd "$(dirname "$0")"
./tools/export.sh Web
python3 tools/sync_web_build.py
