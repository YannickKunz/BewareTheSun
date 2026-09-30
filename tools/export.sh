#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
GODOT="${GODOT:-godot}"
case "${1:-Web}" in
  Web) preset=Web ;;
  macOS) preset=macOS ;;
  *) printf 'Usage: %s [Web|macOS]\n' "$0" >&2; exit 2 ;;
esac
mkdir -p game/exports/web
# Never recursively import or package previous exports into the next build.
touch game/exports/.gdignore
"$GODOT" --headless --path game --editor --import --quit
exec "$GODOT" --headless --path game --export-release "$preset"
