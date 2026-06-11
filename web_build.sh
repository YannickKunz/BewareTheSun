#!/bin/bash
# Web build script for Reino de Aragon using Emscripten
# Prerequisites: Install Emscripten SDK (see README below)
#
# To install Emscripten:
#   cd ~
#   git clone https://github.com/emscripten-core/emsdk.git
#   cd emsdk
#   ./emsdk install latest
#   ./emsdk activate latest
#   source ./emsdk_env.sh
#
# Then run this script from the project root:
#   ./web_build.sh

set -e

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "ERROR: emcc not found. Please install and activate Emscripten SDK first."
    echo ""
    echo "  cd ~"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    echo ""
    echo "Then run this script again."
    exit 1
fi

RAYLIB_SRC="build/external/raylib-master/src"
OUT_DIR="web_build"
RAYLIB_WEB_LIB="$OUT_DIR/libraylib_web.a"

# Create output directory
mkdir -p "$OUT_DIR"

# Step 1: Build raylib for web (if not already built)
if [ ! -f "$RAYLIB_WEB_LIB" ]; then
    echo "=== Building raylib for Emscripten ==="
    
    RAYLIB_SOURCES=(
        "$RAYLIB_SRC/rcore.c"
        "$RAYLIB_SRC/rshapes.c"
        "$RAYLIB_SRC/rtextures.c"
        "$RAYLIB_SRC/rtext.c"
        "$RAYLIB_SRC/rmodels.c"
        "$RAYLIB_SRC/raudio.c"
        "$RAYLIB_SRC/utils.c"
    )
    
    RAYLIB_OBJS=()
    for src in "${RAYLIB_SOURCES[@]}"; do
        obj="$OUT_DIR/$(basename "$src" .c).o"
        echo "  Compiling $(basename "$src")..."
        emcc -c "$src" -o "$obj" \
            -Os \
            -DPLATFORM_WEB \
            -DGRAPHICS_API_OPENGL_ES2 \
            -I"$RAYLIB_SRC" \
            -I"$RAYLIB_SRC/external/glfw/include"
        RAYLIB_OBJS+=("$obj")
    done
    
    echo "  Creating libraylib_web.a..."
    emar rcs "$RAYLIB_WEB_LIB" "${RAYLIB_OBJS[@]}"
    
    # Cleanup object files
    rm -f "${RAYLIB_OBJS[@]}"
    
    echo "  raylib web build complete!"
else
    echo "=== raylib web library already built ==="
fi

# Step 2: Build the game for web
echo "=== Building Reino de Aragon for Web ==="

emcc -o "$OUT_DIR/index.html" \
    src/main.cpp \
    src/Game.cpp \
    src/entities/Player.cpp \
    src/entities/Roach.cpp \
    src/entities/Spider.cpp \
    src/world/Level.cpp \
    src/world/LevelGenerator.cpp \
    src/gfx/ProcArt.cpp \
    -Os -Wall \
    "$RAYLIB_WEB_LIB" \
    -s ASYNCIFY \
    -s USE_GLFW=3 \
    -s ALLOW_MEMORY_GROWTH \
    -s TOTAL_MEMORY=67108864 \
    --shell-file "src/web_shell/shell.html" \
    -I"$RAYLIB_SRC" \
    -Isrc/ \
    -DPLATFORM_WEB \
    --preload-file assets \
    -std=c++17

echo ""
echo "=== Build Complete! ==="
echo "Output files in: $OUT_DIR/"
echo "  - index.html"
echo "  - index.js"  
echo "  - index.wasm"
echo "  - index.data (assets)"
echo ""
echo "To test locally:"
echo "  cd $OUT_DIR && python3 -m http.server 8080"
echo "  Then open http://localhost:8080"
echo ""
echo "To deploy to Netlify:"
echo "  1. Go to app.netlify.com"
echo "  2. Drag-and-drop the '$OUT_DIR' folder"
echo "  Or use netlify-cli: npx netlify-cli deploy --dir=$OUT_DIR --prod"
