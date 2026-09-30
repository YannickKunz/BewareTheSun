#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
GODOT="${GODOT:-godot}"
"$GODOT" --headless --path game --editor --import --quit
"$GODOT" --headless --path game --script tests/test_game.gd -- --test-mode
"$GODOT" --headless --path game --script tests/test_art.gd
