/**
 * @file main.c
 * @brief Entry point for the Chub Game Hub.
 *
 * Initializes the window, audio device, and starts the main loop.
 * Compatible with both native (desktop) and web (Emscripten) targets.
 *
 * On the web, emscripten_set_main_loop controls the loop.
 * On desktop, a standard while loop is used.
 */
#include <stdio.h>

#include "raylib/raylib.h"
#include "hub.h"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

// ── Canvas resolution (adapts to browser on web) ──
#define CANVAS_WIDTH  960
#define CANVAS_HEIGHT 720

// ── Forward declaration ──
static void main_loop(void);

int main(void) {

    // Window configuration
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "CHUB - Game Hub");
    SetTargetFPS(60);
    SetExitKey(0);  // Disable default ESC exit (hub handles ESC)

    // Global audio device — games load/unload their own sounds,
    // but the device itself is managed here once.
    InitAudioDevice();

    // Initialize hub
    hub_init();

    printf("[CHUB] Game Hub initialized (%dx%d)\n", CANVAS_WIDTH, CANVAS_HEIGHT);

    #ifdef PLATFORM_WEB
        // Web: Emscripten controls the loop
        emscripten_set_main_loop(main_loop, 0, 1);
    #else
        // Desktop: standard loop
        while (!WindowShouldClose()) {
            main_loop();
        }
    #endif

    // Cleanup
    hub_cleanup();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

/**
 * @brief Single frame of the main loop (called every frame).
 */
static void main_loop(void) {
    hub_update();

    BeginDrawing();
        hub_draw();
    EndDrawing();
}
