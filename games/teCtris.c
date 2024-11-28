#include "teCtris.h"
#include "raylib.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>




#define GRID_WIDTH 10
#define GRID_HEIGHT 20
#define BLOCK_SIZE 30

typedef struct {
    int x, y;
    int shape[4][4];
    int rotation;
    int colorIndex;
} Tetromino;



Color tetrominoColorsOut[8];
Color tetrominoColorsIn[8];
int score = 0;
int highscore = 0;

// Function prototype
bool CollisionAt(int yOffset);



int grid[GRID_HEIGHT][GRID_WIDTH] = {0};

// Tetromino shapes
const int TETROMINO_I[4][4] = {{0, 0, 0, 0},
                               {1, 1, 1, 1},
                               {0, 0, 0, 0},
                               {0, 0, 0, 0}};

const int TETROMINO_O[4][4] = {{1, 1, 0, 0},
                               {1, 1, 0, 0},
                               {0, 0, 0, 0},
                               {0, 0, 0, 0}};

const int TETROMINO_T[4][4] = {{0, 1, 0, 0},
                               {1, 1, 1, 0},
                               {0, 0, 0, 0},
                               {0, 0, 0, 0}};

const int TETROMINO_L[4][4] = {{0, 0, 1, 0},
                               {1, 1, 1, 0},
                               {0, 0, 0, 0},
                               {0, 0, 0, 0}};

const int TETROMINO_J[4][4] = {{1, 0, 0, 0},
                               {1, 1, 1, 0},
                               {0, 0, 0, 0},
                               {0, 0, 0, 0}};

const int TETROMINO_S[4][4] = {{0, 1, 1, 0},
                               {1, 1, 0, 0},
                               {0, 0, 0, 0},
                               {0, 0, 0, 0}};

const int TETROMINO_Z[4][4] = {{1, 1, 0, 0},
                               {0, 1, 1, 0},
                               {0, 0, 0, 0},
                               {0, 0, 0, 0}};

const int TETROMINO_Q[4][4] = {{1, 1, 1, 0},
                               {1, 0, 1, 0},
                               {1, 1, 1, 0},
                               {0, 1, 0, 0}};

Tetromino currentTetromino;

// Array of all tetromino shapes
const int (*tetrominoShapes[])[4] = {TETROMINO_I, TETROMINO_O, TETROMINO_T, TETROMINO_L, TETROMINO_J, TETROMINO_S, TETROMINO_Z, TETROMINO_Q};


// Function prototypes
void SpawnTetromino();
void HandleInput();
bool Collision();
void LockTetromino();
void ClearLines();
void DrawGame();
void RotateTetromino();
void RotateTetrominoBack();

// Função para salvar o *highscore* em um arquivo
static void SaveHighscore(int highscore) {
    FILE *file = fopen("resources/TectrisHighscore.txt", "w");
    if (file != NULL) {
        fprintf(file, "%d\n", highscore);
        fclose(file);
        printf("Highscore salvo: %d\n", highscore);
    } else {
        printf("Erro ao salvar o highscore.\n");
    }
}

// Função para carregar o *highscore* do arquivo
static int LoadHighscore() {
    FILE *file = fopen("resources/TectrisHighscore.txt", "r");
    int highscore = 0;

    if (file != NULL) {
        fscanf(file, "%d", &highscore);
        fclose(file);
        printf("Highscore carregado: %d\n", highscore);
    } else {
        printf("Nenhum highscore salvo encontrado.\n");
    }

    return highscore;
}

void SpawnTetromino() {
    // Criação de uma tabela de probabilidades
    const int probabilities[] = {
        0, 0, 0, 0, 0, 0, 0, 0, // Ten 0s
        1, 1, 1, 1, 1, 1, 1, 1, // Ten 1s
        2, 2, 2, 2, 2, 2, 2, 2, // Ten 2s
        3, 3, 3, 3, 3, 3, 3, 3, // Ten 3s
        4, 4, 4, 4, 4, 4, 4, 4, // Ten 4s
        5, 5, 5, 5, 5, 5, 5, 5, // Ten 5s
        6, 6, 6, 6, 6, 6, 6, 6, // Ten 6s
        7
        };

    // TETROMINO_Q (índice 7) aparece 1 vez, enquanto outros aparecem mais vezes.
    int size = sizeof(probabilities) / sizeof(probabilities[0]);

    // Escolha do índice com base na probabilidade
    int shapeIndex = probabilities[rand() % size];
    
    currentTetromino = (Tetromino){GRID_WIDTH / 2 - 2, 0, {{0}}, 0, shapeIndex};
    memcpy(currentTetromino.shape, tetrominoShapes[shapeIndex], sizeof(currentTetromino.shape));
}


void HandleInput() {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        currentTetromino.x -= 1;
        if (Collision()) currentTetromino.x += 1;  // Undo if collides
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        currentTetromino.x += 1;
        if (Collision()) currentTetromino.x -= 1;  // Undo if collides
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        RotateTetromino();
        if (Collision()) RotateTetrominoBack();  // Undo if collides
    }
}

bool Collision() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino.shape[i][j]) {
                int x = currentTetromino.x + j;
                int y = currentTetromino.y + i;
                if (x < 0 || x >= GRID_WIDTH || y >= GRID_HEIGHT || (y >= 0 && grid[y][x])) {
                    return true;
                }
            }
        }
    }
    return false;
}

void LockTetromino() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino.shape[i][j]) {
                int x = currentTetromino.x + j;
                int y = currentTetromino.y + i;
                if (y >= 0) grid[y][x] = currentTetromino.colorIndex + 1;  // Store color index
            }
        }
    }
}


bool CollisionAt(int yOffset) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino.shape[i][j]) {
                int x = currentTetromino.x + j;
                int y = yOffset + i;
                if (x < 0 || x >= GRID_WIDTH || y >= GRID_HEIGHT || (y >= 0 && grid[y][x])) {
                    return true;
                }
            }
        }
    }
    return false;
}



void ClearLines() {
    int linesCleared = 0;

    for (int i = GRID_HEIGHT - 1; i >= 0; i--) {
        bool fullLine = true;
        for (int j = 0; j < GRID_WIDTH; j++) {
            if (!grid[i][j]) {
                fullLine = false;
                break;
            }
        }
        if (fullLine) {
            linesCleared++;
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < GRID_WIDTH; j++) {
                    grid[k][j] = grid[k - 1][j];
                }
            }
            for (int j = 0; j < GRID_WIDTH; j++) {
                grid[0][j] = 0;  // Clear the topmost row
            }
            i++;  // Recheck this row
        }
    }

    // Update score based on the number of cleared lines
    if (linesCleared == 1) score += 1;
    else if (linesCleared == 2) score += 3;
    else if (linesCleared == 3) score += 5;
    else if (linesCleared == 4) score += 10;
}

void DrawPreview() {
    // Calculate the preview position based on the current falling tetromino's position
    int previewY = currentTetromino.y;

    // Simulate falling through the grid to find the final resting position
    while (!CollisionAt(previewY + 1)) {
        previewY++;
    }

    // Draw the preview tetromino at the calculated position
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino.shape[i][j]) {
                int x = (currentTetromino.x + j) * BLOCK_SIZE;
                int y = (previewY + i) * BLOCK_SIZE;

                // Draw the preview block with the correct color
                DrawRectangle(x, y, BLOCK_SIZE, BLOCK_SIZE, DARKGRAY);
                DrawRectangle(x + 5, y + 5, 20, 20, GRAY);
            }
        }
    }
}

// Inside the DrawGame function, call DrawPreview before rendering the active tetromino
void DrawGame() {
    BeginDrawing();
    ClearBackground(BLACK);

    for (int i = 0; i <= GetScreenHeight(); i += BLOCK_SIZE * 2) {
        DrawRectangle(0, i, GetScreenWidth(), BLOCK_SIZE, THEGRAY);
    }

    // Draw grid blocks
    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH; j++) {
            if (grid[i][j]) {
                DrawRectangle(j * BLOCK_SIZE, i * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, tetrominoColorsOut[grid[i][j] - 1]);
                DrawRectangle(j * BLOCK_SIZE + 5, i * BLOCK_SIZE + 5, 20, 20, tetrominoColorsIn[grid[i][j] - 1]);
            }
        }
    }

    // Draw the preview of the next Tetromino
    DrawPreview();

    // Draw the current falling tetromino
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino.shape[i][j]) {
                int x = (currentTetromino.x + j) * BLOCK_SIZE;
                int y = (currentTetromino.y + i) * BLOCK_SIZE;
                DrawRectangle(x, y, BLOCK_SIZE, BLOCK_SIZE, tetrominoColorsOut[currentTetromino.colorIndex]);
                DrawRectangle(x + 5, y + 5, 20, 20, tetrominoColorsIn[currentTetromino.colorIndex]);
            }
        }
    }

    // Draw the score
    DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
    DrawText(TextFormat("High: %d", highscore), GetScreenWidth() - MeasureText(TextFormat("High: %d", highscore), 20) - 10, 10, 20, WHITE);
    DrawText(TextFormat("©PietroTy 2024"), 10, 575, 15, WHITE);


    EndDrawing();
}





void RotateTetromino() {

     // Special handling for TETROMINO_Q (index 7)
    if (currentTetromino.colorIndex == 7) {
        int temp[4][4] = {{0}};

        // Perform a 90-degree clockwise rotation while preserving symmetry
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                temp[j][3 - i] = currentTetromino.shape[i][j];
            }
        }

        // Copy the rotated shape back to the current tetromino
        memcpy(currentTetromino.shape, temp, sizeof(temp));
        return;
    }
    // If the current tetromino is the O tetromino, skip rotation
    if (currentTetromino.colorIndex == 1) {  // colorIndex 1 corresponds to TETROMINO_O
        return;
    }

    // Special handling for I shape rotation (index 0)
    if (currentTetromino.colorIndex == 0) {  // colorIndex 0 corresponds to TETROMINO_I
        // If the I shape is vertical (rotation == 0 or 1), rotate it to horizontal, and vice versa
        if (currentTetromino.rotation % 2 == 0) {  // Horizontal
            // Set the I shape to be vertical
            currentTetromino.shape[0][1] = 1;
            currentTetromino.shape[1][1] = 1;
            currentTetromino.shape[2][1] = 1;
            currentTetromino.shape[3][1] = 1;
            currentTetromino.shape[1][0] = 0;
            currentTetromino.shape[1][2] = 0;
            currentTetromino.shape[1][3] = 0;
        } else {  // Vertical
            // Set the I shape to be horizontal
            currentTetromino.shape[0][1] = 0;
            currentTetromino.shape[2][1] = 0;
            currentTetromino.shape[3][1] = 0;
            currentTetromino.shape[1][0] = 1;
            currentTetromino.shape[1][1] = 1;
            currentTetromino.shape[1][2] = 1;
            currentTetromino.shape[1][3] = 1;
        }
        // Toggle rotation state (horizontal/vertical)
        currentTetromino.rotation = (currentTetromino.rotation + 1) % 2;
        return;
    }

    // Normal rotation for other tetrominos
    int temp[4][4] = {{0}}; 

    // Center of the tetromino (for a 4x4 block, the center is at (1,1))
    int centerX = 1;
    int centerY = 1;

    // Perform a 90-degree clockwise rotation around the center (1,1)
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino.shape[i][j]) {
                // Calculate the new positions relative to the center
                int newX = centerY - j + centerX;
                int newY = i - centerX + centerY;

                // Make sure the new position is within bounds of the 4x4 grid
                if (newX >= 0 && newX < 4 && newY >= 0 && newY < 4) {
                    temp[newX][newY] = currentTetromino.shape[i][j];
                }
            }
        }
    }

    memcpy(currentTetromino.shape, temp, sizeof(temp));
}

void RotateTetrominoBack() {

    // Special handling for TETROMINO_Q (index 7)
    if (currentTetromino.colorIndex == 7) {
        int temp[4][4] = {{0}};

        // Perform a 90-degree counter-clockwise rotation while preserving symmetry
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                temp[3 - j][i] = currentTetromino.shape[i][j];
            }
        }

        // Copy the rotated shape back to the current tetromino
        memcpy(currentTetromino.shape, temp, sizeof(temp));
        return;
    }

    // If the current tetromino is the O tetromino, skip rotation
    if (currentTetromino.colorIndex == 1) {  // colorIndex 1 corresponds to TETROMINO_O
        return;
    }

    // Special handling for I shape rotation (index 0)
    if (currentTetromino.colorIndex == 0) {  // colorIndex 0 corresponds to TETROMINO_I
        // If the I shape is vertical (rotation == 0 or 1), rotate it to horizontal, and vice versa
        if (currentTetromino.rotation % 2 == 0) {  // Horizontal
            // Set the I shape to be vertical
            currentTetromino.shape[0][1] = 1;
            currentTetromino.shape[1][1] = 1;
            currentTetromino.shape[2][1] = 1;
            currentTetromino.shape[3][1] = 1;
            currentTetromino.shape[1][0] = 0;
            currentTetromino.shape[1][2] = 0;
            currentTetromino.shape[1][3] = 0;
        } else {  // Vertical
            // Set the I shape to be horizontal
            currentTetromino.shape[0][1] = 0;
            currentTetromino.shape[2][1] = 0;
            currentTetromino.shape[3][1] = 0;
            currentTetromino.shape[1][0] = 1;
            currentTetromino.shape[1][1] = 1;
            currentTetromino.shape[1][2] = 1;
            currentTetromino.shape[1][3] = 1;
        }
        // Toggle rotation state (horizontal/vertical)
        currentTetromino.rotation = (currentTetromino.rotation + 1) % 2;
        return;
    }

    // Normal rotation for other tetrominos
    int temp[4][4] = {{0}}; 

    // Center of the tetromino (for a 4x4 block, the center is at (1,1))
    int centerX = 1;
    int centerY = 1;

    // Perform a 90-degree counter-clockwise rotation around the center (1,1)
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino.shape[i][j]) {
                // Calculate the new positions relative to the center
                int newX = j - centerY + centerX;
                int newY = centerX - i + centerY;

                // Make sure the new position is within bounds of the 4x4 grid
                if (newX >= 0 && newX < 4 && newY >= 0 && newY < 4) {
                    temp[newX][newY] = currentTetromino.shape[i][j];
                }
            }
        }
    }

    memcpy(currentTetromino.shape, temp, sizeof(temp));
}

static void ResetGame(int *score, bool *gameOver) {
    // Reset score and game over state
    *score = 0;
    *gameOver = false;

    // Reset grid (set all values to 0)
    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH; j++) {
            grid[i][j] = 0;
        }
    }

    // Spawn a new tetromino
    SpawnTetromino();
}



GameState teCtris() {
    // Game logic for snaCke here
    // Example placeholder logic:
    while (1) {
        if (IsKeyPressed(KEY_ESCAPE)) return MENU;  // Exit to menu

                // Atribuir os valores ao array dentro de main
                tetrominoColorsOut[0] = DARKBLUE;
                tetrominoColorsOut[1] = DARKGREEN;
                tetrominoColorsOut[2] = DARKPURPLE;
                tetrominoColorsOut[3] = DARKYELLOW;
                tetrominoColorsOut[4] = DARKSKY;
                tetrominoColorsOut[5] = DARKORANGE;
                tetrominoColorsOut[6] = DARKRED;
                tetrominoColorsOut[7] = LIGHTGRAY;
                tetrominoColorsIn[0] = BLUE;
                tetrominoColorsIn[1] = GREEN;
                tetrominoColorsIn[2] = PURPLE;
                tetrominoColorsIn[3] = YELLOW;
                tetrominoColorsIn[4] = SKYBLUE;
                tetrominoColorsIn[5] = ORANGE;
                tetrominoColorsIn[6] = RED;
                tetrominoColorsIn[7] = WHITE;

                SetWindowIcon(LoadImage("resources/iconeTectris.png"));
                SetTargetFPS(60);

                srand(time(NULL));  // Seed for random tetromino spawning
                float fallInterval = 0.5f;  // Normal falling speed
                float lastFallTime = GetTime();
                highscore = LoadHighscore();


                bool gameOver = false;
                

                SpawnTetromino();

                while (!WindowShouldClose()) {

                    if (gameOver) {

                        if (score > highscore) {
                            highscore = score;
                            SaveHighscore(highscore); // Salva o novo *highscore*
                        }

                            if (IsKeyPressed(KEY_R)) {
                                ResetGame(&score, &gameOver);
                            }


                            BeginDrawing();
                            ClearBackground(BLACK);
                            const char *lostMessage = "You Lost!";
                            const char *scoreMessage = TextFormat("Score: %d", score);
                            const char *highscoreMessage = TextFormat("Highscore: %d", highscore);
                            const char *resetMessage = "press R to reset";
                            DrawText(lostMessage, (GetScreenWidth() - MeasureText(lostMessage, 50)) / 2, (GetScreenHeight() - 400) / 2, 50, WHITE);
                            DrawText(scoreMessage, (GetScreenWidth() - MeasureText(scoreMessage, 40)) / 2, (GetScreenHeight()) / 2, 40, WHITE);
                            DrawText(highscoreMessage, (GetScreenWidth() - MeasureText(highscoreMessage, 30)) / 2, (GetScreenHeight() + 100) / 2, 30, WHITE);
                            DrawText(resetMessage, (GetScreenWidth() - MeasureText(resetMessage, 20)) / 2, (GetScreenHeight() + 300) / 2, 20, WHITE);
                            
                            DrawText(TextFormat("©PietroTy 2024"), 10, 570, 20, WHITE);

                            EndDrawing();

                            continue;
                        }
                    
                    float currentTime = GetTime();

                    // Check if the DOWN key is being held
                    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
                        fallInterval = 0.05f;  // Fall twice as fast when the DOWN key is held
                    } else {
                        fallInterval = 0.5f;  // Normal fall speed
                    }

                    HandleInput();

                    if (currentTime - lastFallTime >= fallInterval) {
                        lastFallTime = currentTime;
                        currentTetromino.y += 1;
                        if (Collision()) {
                            currentTetromino.y -= 1;
                            LockTetromino();
                            ClearLines();
                            SpawnTetromino();
                            if (Collision()) {
                                // Game Over
                                gameOver = true;
                            }
                        }
                    }

                    DrawGame();
                }

    }

    return MENU;
}