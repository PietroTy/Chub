#!/bin/bash

# =============================================================================
# buildwasm.sh — Build Chub Game Hub for WebAssembly (Linux version)
# =============================================================================

PROJECT_NAME="chub"
BUILD_DIR="web"
SERVER_PORT=8080

# Function to display help
show_help() {
    echo "Usage: ./buildwasm.sh [options]"
    echo "Options:"
    echo "  -clean             Clear build artifacts"
    echo "  -compile           Compile only"
    echo "  -run               Start local server (must be compiled)"
    echo "  -compileAndRun     Compile and start local server"
    echo "  (no options)       Clean, compile, and run"
}

# Parse arguments
CLEAN=false
COMPILE=false
RUN=false

if [ $# -eq 0 ]; then
    CLEAN=true
    COMPILE=true
    RUN=true
else
    for arg in "$@"; do
        case $arg in
            -clean) CLEAN=true ;;
            -compile) COMPILE=true ;;
            -run) RUN=true ;;
            -compileAndRun) COMPILE=true; RUN=true ;;
            -h|--help) show_help; exit 0 ;;
            *) echo "Unknown option: $arg"; show_help; exit 1 ;;
        esac
    done
fi

# Check for emcc
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: emcc not found. Please activate Emscripten (source ~/emsdk/emsdk_env.sh)"
    exit 1
fi

# ── Clean ──
if [ "$CLEAN" = true ]; then
    echo "🧹 Cleaning..."
    rm -rf "$BUILD_DIR"
    if [ "$COMPILE" = false ] && [ "$RUN" = false ]; then exit 0; fi
fi

# ── Compile ──
if [ "$COMPILE" = true ]; then
    echo "🔨 Compiling for WebAssembly..."
    mkdir -p "$BUILD_DIR"

    # Ensure assets directories exist (simulating the ps1 logic)
    mkdir -p assets/shared assets/pong assets/crappybird assets/snake assets/tectris
    touch assets/shared/.gitkeep assets/pong/.gitkeep assets/crappybird/.gitkeep assets/snake/.gitkeep assets/tectris/.gitkeep

    # Source files
    SOURCES=(
        "./core/main.c"
        "./core/hub.c"
        "./core/registry.c"
        "./games/candycrush/candycrush.c"
        "./games/cballpool/cballpool.c"
        "./games/crappybird/crappybird.c"
        "./games/crazyjump/crazyjump.c"
        "./games/ctron/ctron.c"
        "./games/mortalcombat/mortalcombat.c"
        "./games/ponc/ponc.c"
        "./games/snacke/snacke.c"
        "./games/solitairecpider/solitairecpider.c"
        "./games/spaceinvaders/spaceinvaders.c"
        "./games/tectris/tectris.c"
    )

    emcc "${SOURCES[@]}" \
        -o "./$BUILD_DIR/index.html" \
        -Wall \
        -std=gnu99 \
        -D_DEFAULT_SOURCE \
        -Wno-missing-braces \
        -Os \
        -I ./core/include \
        -I ./core/include/raylib \
        -L ./lib/wasm/ \
        -s USE_GLFW=3 \
        -s ASYNCIFY \
        -s INITIAL_MEMORY=67108864 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s FORCE_FILESYSTEM=1 \
        --preload-file ./assets@/assets \
        --shell-file ./platform/web/shell.html \
        "./lib/wasm/libraylib.a" \
        -DPLATFORM_WEB \
        -s 'EXPORTED_FUNCTIONS=["_free","_malloc","_main"]' \
        -s EXPORTED_RUNTIME_METHODS=ccall

    if [ $? -eq 0 ]; then
        echo "✅ Build successful! Output in $BUILD_DIR/"
    else
        echo "❌ Build failed!"
        exit 1
    fi
fi

# ── Run ──
if [ "$RUN" = true ]; then
    HTML_FILE="$BUILD_DIR/$PROJECT_NAME.html"
    if [ ! -f "$HTML_FILE" ]; then
        echo "❌ $HTML_FILE not found! Compile first."
        exit 1
    fi

    echo "🌐 Starting local server on http://localhost:$SERVER_PORT/$PROJECT_NAME.html"
    echo "   Press Ctrl+C to stop"
    
    cd "$BUILD_DIR" || exit
    python3 -m http.server "$SERVER_PORT"
fi
