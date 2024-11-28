#include "raylib.h"
#include "shared.h"
#include "games/teCtris.h"
#include "games/Crappybird.h"
#include "games/snaCke.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Game Hub");
    SetTargetFPS(60);

    GameState currentState = MENU;  // Start in the main menu

    while (currentState != EXIT) {  // Main loop only exits on EXIT state
        switch (currentState) {
            case MENU: {
                BeginDrawing();
                ClearBackground(RAYWHITE);

                DrawText("Game Hub", 350, 100, 30, BLACK);
                DrawText("1. Play teCtris", 300, 200, 20, DARKGRAY);
                DrawText("2. Play Crappybird", 300, 250, 20, DARKGRAY);
                DrawText("3. Play snaCke", 300, 300, 20, DARKGRAY);
                DrawText("Esc to Exit", 300, 350, 20, DARKGRAY);

                EndDrawing();

                if (IsKeyPressed(KEY_ONE)) currentState = TECTRIS;
                else if (IsKeyPressed(KEY_TWO)) currentState = CRAPPYBIRD;
                else if (IsKeyPressed(KEY_THREE)) currentState = SNAKE;
                else if (IsKeyPressed(KEY_ESCAPE)) currentState = EXIT;

                break;
            }
            case TECTRIS:
                currentState = teCtris();  // Run teCtris and return the next state
                break;
            case CRAPPYBIRD:
                currentState = Crappybird();  // Run Crappybird and return the next state
                break;
            case SNAKE:
                currentState = snaCke();  // Run snaCke and return the next state
                break;
            case EXIT:
                break;  // Exit state explicitly terminates the loop
        }
    }

    CloseWindow();  // Close the window before exiting
    return 0;
}
