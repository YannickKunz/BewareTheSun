#!/bin/sh
set -eu
cd "$(dirname "$0")"
GODOT="${GODOT:-godot}"
if ! command -v "$GODOT" >/dev/null 2>&1; then
  GODOT="/Applications/Godot.app/Contents/MacOS/Godot"
fi
# Import source assets before standalone play, including on a fresh clone.
"$GODOT" --headless --path game --editor --import --quit
exec "$GODOT" --path game "$@"
