#include "Crappybird.h"
#include "raylib.h"




#include <stdio.h>
#include <raylib.h>
#include <math.h>

// Estrutura para representar o pássaro
typedef struct Bird {
    float x;
    float y;
    float g;
    bool flaping;
    int color; // Cor do pássaro
} Bird;

// Estrutura para representar os canos
typedef struct Pipe {
    int x;
    int height;
    bool isActive;
} Pipe;

// Define the cloud structure
typedef struct Cloud {
    float x, y;  // Position of the cloud
    Color color; // Color of the cloud
    
} Cloud;


static void ResetGame(Bird *bird, Pipe *pipe, int *score, int *speed, bool *gameOver, int *lastScore) {
    // Reiniciar pássaro
    bird->x = (GetScreenWidth() - 300) / 2; // Posição inicial do pássaro
    bird->y = GetScreenHeight() / 2;
    bird->g = 0;
    bird->flaping = false;

    // Reiniciar canos
    for (int i = 0; i < 2; i++) {
        pipe[i].x = 400 + i * 300; // Distância entre os canos
        pipe[i].height = GetRandomValue(100, GetScreenHeight() - 250); // Altura aleatória para os canos
        pipe[i].isActive = true;
    }

    // Reiniciar placar e estado do jogo
    *score = 0;
    *speed = 2;
    *gameOver = false;
    *lastScore = 0; // Reinicia o lastScore para evitar problemas no aumento de velocidade
}

// Função para interpolar entre duas cores
Color LerpColor(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + t * (b.r - a.r)),
        (unsigned char)(a.g + t * (b.g - a.g)),
        (unsigned char)(a.b + t * (b.b - a.b)),
        255 // Alpha fixo
    };
}

// Função para salvar o *highscore* em um arquivo
static void SaveHighscore(int highscore) {
    FILE *file = fopen("resources/CrappyHighscore.txt", "w");
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
    FILE *file = fopen("resources/CrappyHighscore.txt", "r");
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

GameState Crappybird() {
    // Game logic for Crappybird here
    // Example placeholder logic:
    while (1) {
        if (IsKeyPressed(KEY_ESCAPE)) return MENU;  // Exit to menu

            Bird bird;
            Pipe pipe[2]; // Dois canos simultâneos na tela
            int score = 0;
            int speed = 2;
            bool gameOver = true;
            bool store = false;
            int lastScore = 0; // Nova variável para rastrear a última pontuação que aumentou a velocidade
            int highscore = LoadHighscore(); // Carrega o *highscore* ao iniciar o jogo
            bird.color = 1;

            SetWindowIcon(LoadImage("resources/iconeCrappy.png"));
            SetTargetFPS(60);

            ResetGame(&bird, pipe, &score, &speed, &gameOver, &lastScore);
            
            float startTime = GetTime(); // Tempo inicial do jogo

            // Define an array of 3 clouds
            Cloud clouds[6];

            // Initialize clouds with random heights
            for (int i = 0; i < 6; i++) {
                clouds[i].x = i * 100;  // Spread them out a bit initially
                clouds[i].y = GetRandomValue(100, GetScreenHeight());    // Randomize height between 50 and 200
                clouds[i].color = WHITE;                  // Set cloud color (can be changed)
            }

            float cloudSpeed = 1.0f;  // Speed of the clouds (slower than the pipes)


            while (!WindowShouldClose()) {
                // Atualização do tempo e lógica do jogo
                if (gameOver) {

                    if (store) {

                        if (IsKeyPressed(KEY_S)) {
                            store = false;
                        }
                        if (IsKeyPressed(KEY_KP_1) || IsKeyPressed(KEY_ONE)) {
                            bird.color = 1;
                        } else if ((IsKeyPressed(KEY_KP_2) || IsKeyPressed(KEY_TWO)) && highscore >= 15) {
                            bird.color = 2;
                        } else if ((IsKeyPressed(KEY_KP_3) || IsKeyPressed(KEY_THREE)) && highscore >= 20) {
                            bird.color = 3;
                        } else if ((IsKeyPressed(KEY_KP_4) || IsKeyPressed(KEY_FOUR)) && highscore >= 25) {
                            bird.color = 4;
                        } else if ((IsKeyPressed(KEY_KP_5) || IsKeyPressed(KEY_FIVE)) && highscore >= 30) {
                            bird.color = 5;
                        } else if ((IsKeyPressed(KEY_KP_6) || IsKeyPressed(KEY_SIX)) && highscore >= 35) {
                            bird.color = 6;
                        } else if ((IsKeyPressed(KEY_KP_7) || IsKeyPressed(KEY_SEVEN)) && highscore >= 40) {
                            bird.color = 7;
                        } else if ((IsKeyPressed(KEY_KP_8) || IsKeyPressed(KEY_EIGHT)) && highscore >= 45) {
                            bird.color = 8;
                        } else if ((IsKeyPressed(KEY_KP_9) || IsKeyPressed(KEY_NINE)) && highscore >= 50) {
                            bird.color = 9;
                        }
                        if (bird.color == 1) {
                            DrawRectangle((GetScreenWidth() / 2) - 200, (GetScreenHeight() / 2) - 55, 20, 20, BLACK);
                        } else if (bird.color == 2) {
                            DrawRectangle((GetScreenWidth() / 2) - 50, (GetScreenHeight() / 2) - 55, 20, 20, BLACK);
                        } else if (bird.color == 3) {
                            DrawRectangle((GetScreenWidth() / 2) + 100, (GetScreenHeight() / 2) - 55, 20, 20, BLACK);
                        } else if (bird.color == 4) {
                            DrawRectangle((GetScreenWidth() / 2) - 200, (GetScreenHeight() / 2) + 45, 20, 20, BLACK);
                        } else if (bird.color == 5) {
                            DrawRectangle((GetScreenWidth() / 2) - 50, (GetScreenHeight() / 2) + 45, 20, 20, BLACK);
                        } else if (bird.color == 6) {
                            DrawRectangle((GetScreenWidth() / 2) + 100, (GetScreenHeight() / 2) + 45, 20, 20, BLACK);
                        } else if (bird.color == 7) {
                            DrawRectangle((GetScreenWidth() / 2) - 200, (GetScreenHeight() / 2) + 145, 20, 20, BLACK);
                        } else if (bird.color == 8) {
                            DrawRectangle((GetScreenWidth() / 2) - 50, (GetScreenHeight() / 2) + 145, 20, 20, BLACK);
                        } else if (bird.color == 9) {
                            DrawRectangle((GetScreenWidth() / 2) + 100, (GetScreenHeight() / 2) + 145, 20, 20, BLACK);
                        }

                        BeginDrawing();
                        ClearBackground(THEGRAY);
                        const char *storeMessage = "Store";
                        const char *highscoreMessage = TextFormat("Highscore: %d", highscore);
                        const char *resetMessage = "press S to close store";
                        DrawText(storeMessage, (GetScreenWidth() - MeasureText(storeMessage, 100)) / 2, (GetScreenHeight() - 550) / 2, 100, WHITE);
                        DrawText(highscoreMessage, (GetScreenWidth() - MeasureText(highscoreMessage, 60)) / 2, (GetScreenHeight() - 350) / 2, 60, WHITE);
                        DrawText(resetMessage, (GetScreenWidth() - MeasureText(resetMessage, 30)) / 2, (GetScreenHeight() + 390) / 2, 30, WHITE);

                        // Desenhar pássaro 1
                        DrawRectangle((GetScreenWidth() / 2) - 170, (GetScreenHeight() / 2) - 80, 40, 40, YELLOW);
                        DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) - 70, 20, 20, WHITE);
                        DrawRectangle((GetScreenWidth() / 2) - 150, (GetScreenHeight() / 2) - 70, 10, 10, BLACK);
                        DrawRectangle((GetScreenWidth() / 2) - 140, (GetScreenHeight() / 2) - 60, 20, 10, ORANGE);
                        DrawRectangle((GetScreenWidth() / 2) - 185, (GetScreenHeight() / 2) - 70, 30, 20, ORANGE);
                        DrawText("1", (GetScreenWidth() / 2) - 195, (GetScreenHeight() / 2) - 55, 20, WHITE);


                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) - 70, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 5, (GetScreenHeight() / 2) - 80, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) - 75, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 5, (GetScreenHeight() / 2) - 75, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 5, (GetScreenHeight() / 2) - 65, 10, 10, THEGRAY);

                        if (highscore >= 15) {
                            // Desenhar pássaro 2
                            DrawRectangle((GetScreenWidth() / 2) - 20, (GetScreenHeight() / 2) - 80, 40, 40, GREEN);
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) - 70, 20, 20, WHITE);
                            DrawRectangle((GetScreenWidth() / 2), (GetScreenHeight() / 2) - 70, 10, 10, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) + 10, (GetScreenHeight() / 2) - 60, 20, 10, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) + 15, (GetScreenHeight() / 2) - 50, 5, 5, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) + 20, (GetScreenHeight() / 2) - 50, 10, 10, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) - 35, (GetScreenHeight() / 2) - 70, 30, 20, DARKGREEN);
                            DrawText("2", (GetScreenWidth() / 2) - 45, (GetScreenHeight() / 2) - 55, 20, WHITE);
                        }

                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) - 70, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 145, (GetScreenHeight() / 2) - 80, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) - 75, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 155, (GetScreenHeight() / 2) - 75, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 145, (GetScreenHeight() / 2) - 65, 10, 10, THEGRAY);
                        if (highscore >= 20) {
                            // Desenhar pássaro 3
                            DrawRectangle((GetScreenWidth() / 2) + 130, (GetScreenHeight() / 2) - 80, 40, 40, DARKBLUE);
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) - 70, 20, 20, WHITE);
                            DrawRectangle((GetScreenWidth() / 2) + 150, (GetScreenHeight() / 2) - 70, 10, 10, BLACK);   
                            DrawRectangle((GetScreenWidth() / 2) + 160, (GetScreenHeight() / 2) - 60, 20, 10, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) + 165, (GetScreenHeight() / 2) - 50, 5, 5, BLACK);  
                            DrawRectangle((GetScreenWidth() / 2) + 170, (GetScreenHeight() / 2) - 50, 10, 10, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) + 115, (GetScreenHeight() / 2) - 70, 30, 20, BLUE);
                            DrawText("3", (GetScreenWidth() / 2) + 105, (GetScreenHeight() / 2) - 55, 20, WHITE);
                        }
                            DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) + 30, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 155, (GetScreenHeight() / 2) + 20, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) + 25, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 145, (GetScreenHeight() / 2) + 25, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 155, (GetScreenHeight() / 2) + 35, 10, 10, THEGRAY);
                        if (highscore >= 25) {
                            // Desenhar pássaro 4
                            DrawRectangle((GetScreenWidth() / 2) - 170, (GetScreenHeight() / 2) + 20, 40, 40, WHITE);
                            DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) + 30, 20, 20, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 150, (GetScreenHeight() / 2) + 30, 10, 10, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) - 130, (GetScreenHeight() / 2) + 40, 10, 10, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) + 45, 20, 5, PINK);
                            DrawRectangle((GetScreenWidth() / 2) - 185, (GetScreenHeight() / 2) + 30, 30, 20, WHITE);
                            DrawText("4", (GetScreenWidth() / 2) - 195, (GetScreenHeight() / 2) + 45, 20, WHITE);
                        }
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) + 30, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 5, (GetScreenHeight() / 2) + 20, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) + 25, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 5, (GetScreenHeight() / 2) + 25, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 5, (GetScreenHeight() / 2) + 35, 10, 10, THEGRAY);
                        if (highscore >= 30) {
                            // Desenhar pássaro 5
                            DrawRectangle((GetScreenWidth() / 2) - 20, (GetScreenHeight() / 2) + 20, 40, 40, BROWN);
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) + 30, 20, 20, WHITE);
                            DrawRectangle((GetScreenWidth() / 2), (GetScreenHeight() / 2) + 30, 10, 10, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) + 10, (GetScreenHeight() / 2) + 40, 30, 10, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2), (GetScreenHeight() / 2) + 15, 10, 5, BROWN);
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) + 10, 10, 5, BROWN);
                            DrawRectangle((GetScreenWidth() / 2) - 35, (GetScreenHeight() / 2) + 30, 30, 20, DARKBROWN);
                            DrawText("5", (GetScreenWidth() / 2) - 45, (GetScreenHeight() / 2) + 45, 20, WHITE);
                        }
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) + 30, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 145, (GetScreenHeight() / 2) + 20, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) + 25, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 155, (GetScreenHeight() / 2) + 25, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 145, (GetScreenHeight() / 2) + 35, 10, 10, THEGRAY);
                        if (highscore >= 35) {
                            // Desenhar pássaro 6
                            DrawRectangle((GetScreenWidth() / 2) + 130, (GetScreenHeight() / 2) + 20, 40, 40, PINK);
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) + 30, 20, 20, WHITE);
                            DrawRectangle((GetScreenWidth() / 2) + 150, (GetScreenHeight() / 2) + 30, 10, 10, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) + 160, (GetScreenHeight() / 2) + 40, 20, 10, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) + 165, (GetScreenHeight() / 2) + 50, 5, 5, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) + 170, (GetScreenHeight() / 2) + 50, 10, 10, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) + 115, (GetScreenHeight() / 2) + 30, 30, 20, DARKPURPLE);
                            DrawText("6", (GetScreenWidth() / 2) + 105, (GetScreenHeight() / 2) + 45, 20, WHITE);
                        }
                            DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) + 130, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 155, (GetScreenHeight() / 2) + 120, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) + 125, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 145, (GetScreenHeight() / 2) + 125, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 155, (GetScreenHeight() / 2) + 135, 10, 10, THEGRAY);
                        if (highscore >= 40) {
                            // Desenhar pássaro 7
                            DrawRectangle((GetScreenWidth() / 2) - 170, (GetScreenHeight() / 2) + 120, 40, 40, PURPLE);
                            DrawRectangle((GetScreenWidth() / 2) - 160, (GetScreenHeight() / 2) + 130, 20, 20, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) - 150, (GetScreenHeight() / 2) + 130, 10, 10, WHITE);
                            DrawRectangle((GetScreenWidth() / 2) - 140, (GetScreenHeight() / 2) + 140, 20, 10, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) - 185, (GetScreenHeight() / 2) + 130, 30, 20, DARKBLUE);
                            DrawText("7", (GetScreenWidth() / 2) - 195, (GetScreenHeight() / 2) + 145, 20, WHITE);
                        }
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) + 130, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 5, (GetScreenHeight() / 2) + 120, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) + 125, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 5, (GetScreenHeight() / 2) + 125, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) - 5, (GetScreenHeight() / 2) + 135, 10, 10, THEGRAY);

                        if (highscore >= 45) {
                            // Desenhar pássaro 8
                            DrawRectangle((GetScreenWidth() / 2) - 20, (GetScreenHeight() / 2) + 120, 40, 40, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) - 10, (GetScreenHeight() / 2) + 130, 20, 20, WHITE);
                            DrawRectangle((GetScreenWidth() / 2), (GetScreenHeight() / 2) + 130, 10, 10, RED);
                            DrawRectangle((GetScreenWidth() / 2) + 10, (GetScreenHeight() / 2) + 140, 20, 10, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) - 35, (GetScreenHeight() / 2) + 130, 30, 20, RED);
                            DrawText("8", (GetScreenWidth() / 2) - 45, (GetScreenHeight() / 2) + 145, 20, WHITE);
                        }
                        
                        if (highscore >= 50) {
                            // Desenhar pássaro 9
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) + 130, 20, 20, YELLOW);
                            DrawRectangle((GetScreenWidth() / 2) + 145, (GetScreenHeight() / 2) + 135, 10, 10, WHITE);
                            DrawRectangle((GetScreenWidth() / 2) + 150, (GetScreenHeight() / 2) + 135, 5, 5, BLACK);
                            DrawRectangle((GetScreenWidth() / 2) + 155, (GetScreenHeight() / 2) + 140, 10, 5, ORANGE);
                            DrawRectangle((GetScreenWidth() / 2) + 130, (GetScreenHeight() / 2) + 135, 15, 10, ORANGE);
                            DrawText("9", (GetScreenWidth() / 2) + 105, (GetScreenHeight() / 2) + 145, 20, WHITE);
                        } else {
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) + 130, 20, 20, GRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 145, (GetScreenHeight() / 2) + 120, 10, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 140, (GetScreenHeight() / 2) + 125, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 155, (GetScreenHeight() / 2) + 125, 5, 5, LIGHTGRAY);
                            DrawRectangle((GetScreenWidth() / 2) + 145, (GetScreenHeight() / 2) + 135, 10, 10, THEGRAY);
                        }

                        DrawText(TextFormat("©PietroTy 2024"), 10, 570, 20, WHITE);
                        EndDrawing();
                        continue;




                    } else {

                        if (score > highscore) {
                            highscore = score;
                            SaveHighscore(highscore); // Salva o novo *highscore*
                        }

                        if (IsKeyPressed(KEY_R)) {
                            ResetGame(&bird, pipe, &score, &speed, &gameOver, &lastScore);
                        }

                        if (IsKeyPressed(KEY_S)) {
                            store = true;
                        }

                        BeginDrawing();
                        ClearBackground(BLACK);
                        const char *lostMessage = "You Lost!";
                        const char *scoreMessage = TextFormat("Score: %d", score);
                        const char *highscoreMessage = TextFormat("Highscore: %d", highscore);
                        const char *resetMessage = "press R to reset or S to store";
                        DrawText(lostMessage, (GetScreenWidth() - MeasureText(lostMessage, 100)) / 2, (GetScreenHeight() - 200) / 2, 100, WHITE);
                        DrawText(scoreMessage, (GetScreenWidth() - MeasureText(scoreMessage, 60)) / 2, (GetScreenHeight() + 100) / 2, 60, WHITE);
                        DrawText(highscoreMessage, (GetScreenWidth() - MeasureText(highscoreMessage, 40)) / 2, (GetScreenHeight() + 260) / 2, 40, WHITE);
                        DrawText(resetMessage, (GetScreenWidth() - MeasureText(resetMessage, 30)) / 2, (GetScreenHeight() + 390) / 2, 30, WHITE);
                        
                        DrawText(TextFormat("©PietroTy 2024"), 10, 570, 20, WHITE);
                        EndDrawing();
                        continue;

                    }


                }

                // Gravidade no pássaro
                bird.g += 0.5f; 
                bird.y += bird.g;

                // Controle do pássaro
                if (IsKeyPressed(KEY_SPACE)) {
                    bird.g = -8.0f; // Pulo
                }

                // Verifica se o pássaro está subindo ou caindo
                if (bird.g < 0) {
                    bird.flaping = true;  // Pássaro está subindo
                } else {
                    bird.flaping = false; // Pássaro está caindo
                }

                // Movimento dos canos
                for (int i = 0; i < 2; i++) {
                    pipe[i].x -= speed;

                    if (pipe[i].x < -50) {
                        pipe[i].x = GetScreenWidth();
                        pipe[i].height = GetRandomValue(100, GetScreenHeight() - 250); // Gerar nova altura
                        pipe[i].isActive = true;
                    }
                }

                // Aumenta a velocidade do jogo a cada 5 pontos
                if (score >= lastScore + 5 && speed <= 7) {
                    speed += 1;   // Incrementa a velocidade em 1
                    lastScore = score;  // Atualiza o último marco
                }

                // Verificar colisão com os canos ou com os limites da tela
                for (int i = 0; i < 2; i++) {
                    if (bird.color == 9) {
                        // Adaptação para o pássaro com color == 9 (dimensões 20x20)
                        if ((bird.x + 20 > pipe[i].x && bird.x < pipe[i].x + 50) &&
                            (bird.y < pipe[i].height || bird.y + 20 > pipe[i].height + 150)) {
                            gameOver = true; // Colisão com o cano
                        }
                    } else {
                        // Colisão para outros pássaros (dimensões 40x40)
                        if ((bird.x + 40 > pipe[i].x && bird.x < pipe[i].x + 50) &&
                            (bird.y < pipe[i].height || bird.y + 40 > pipe[i].height + 150)) {
                            gameOver = true; // Colisão com o cano
                        }
                    }
                }


                if (bird.y > GetScreenHeight() || bird.y < 0) gameOver = true; // Colisão com o teto ou o chão

                for (int i = 0; i < 2; i++) {
                    if (pipe[i].isActive && bird.x > pipe[i].x + 50) {
                        score++;
                        pipe[i].isActive = false; // Marca o cano como pontuado
                    }
                }

                // Move each cloud to the left at a slower pace
                for (int i = 0; i < 6; i++) {
                    clouds[i].x -= cloudSpeed;

                    // If the cloud moves off the screen (on the left), reset its position to the right side with random height
                    if (clouds[i].x + 100 < 0) {  // 100 is the width of the cloud
                        clouds[i].x = GetScreenWidth();
                        clouds[i].y = GetRandomValue(100, GetScreenHeight());    // Randomize height between 50 and 200
                    }
                }
                // Alternar gradualmente entre SKYBLUE e DARKBLUE
                float elapsedTime = GetTime() - startTime;
                float cycleTime = 40.0f; // Tempo total para ciclo completo (20s clareando e 20s escurecendo)
                float t = fmod(elapsedTime, cycleTime) / cycleTime; // Valor de interpolação entre 0 e 1

                Color backgroundColor;
                if (t < 0.5f) {
                    // Transição de SKYBLUE para DARKBLUE
                    backgroundColor = LerpColor(SKYBLUE, DEEPBLUE, t * 2);
                } else {
                    // Transição de DARKBLUE para SKYBLUE
                    backgroundColor = LerpColor(DEEPBLUE, SKYBLUE, (t - 0.5f) * 2);
                }

                Color sunColor;
                if (t < 0.5f) {
                    // Transição de SKYBLUE para DARKBLUE
                    sunColor = LerpColor(GOLD, DARKORANGE, t * 2);
                } else {
                    // Transição de DARKBLUE para SKYBLUE
                    sunColor = LerpColor(DARKORANGE, GOLD, (t - 0.5f) * 2);
                }
                Color cloudColor;
                if (t < 0.5f) {
                    // Transição de SKYBLUE para DARKBLUE
                    cloudColor = LerpColor(WHITE, DARKGRAY, t * 2);
                } else {
                    // Transição de DARKBLUE para SKYBLUE
                    cloudColor = LerpColor(DARKGRAY, WHITE, (t - 0.5f) * 2);
                }

                Color pipeColor;
                if (t < 0.5f) {
                    pipeColor = LerpColor(LIME, DARKGREEN, t * 2);
                } else {
                    pipeColor = LerpColor(DARKGREEN, LIME, (t - 0.5f) * 2);
                }

                Color pipeBorderColor;
                if (t < 0.5f) {
                    pipeBorderColor = LerpColor(DARKGREEN, LIME, t * 2);
                } else {
                    pipeBorderColor = LerpColor(LIME, DARKGREEN, (t - 0.5f) * 2);
                }

                // Desenhar
                BeginDrawing();
                ClearBackground(backgroundColor); // Alternar entre fundo claro e escuro

                DrawRectangle(clouds[5].x, 100, 100, 100, sunColor); // Add a fluffy part of the cloud

                // Draw each cloud
                for (int i = 0; i < 6; i++) {
                    DrawRectangle(clouds[i].x, clouds[i].y, 100, 20, cloudColor); // Draw the cloud as a rectangle
                    DrawRectangle(clouds[i].x + 30, clouds[i].y - 20, 40, 20, cloudColor); // Add a fluffy part of the cloud
                }


                // Desenhar pássaro
                if (bird.color == 1){
                    DrawRectangle(bird.x, bird.y, 40, 40, YELLOW);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, WHITE);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, BLACK);
                    DrawRectangle(bird.x + 30, bird.y + 20, 20, 10, ORANGE);
                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, ORANGE);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, ORANGE);
                    }
                } else if (bird.color == 2){
                    DrawRectangle(bird.x, bird.y, 40, 40, GREEN);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, WHITE);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, BLACK);
                    DrawRectangle(bird.x + 30, bird.y + 20, 20, 10, ORANGE);
                    DrawRectangle(bird.x + 35, bird.y + 30, 5, 5, ORANGE);
                    DrawRectangle(bird.x + 40, bird.y + 30, 10, 10, ORANGE);
                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, DARKGREEN);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, DARKGREEN);
                    }
                } else if (bird.color == 3){
                    DrawRectangle(bird.x, bird.y, 40, 40, DARKBLUE);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, WHITE);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, BLACK);
                    DrawRectangle(bird.x + 30, bird.y + 20, 20, 10, BLACK);
                    DrawRectangle(bird.x + 35, bird.y + 30, 5, 5, BLACK);
                    DrawRectangle(bird.x + 40, bird.y + 30, 10, 10, BLACK);

                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, BLUE);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, BLUE);
                    }
                } else if (bird.color == 4){
                    DrawRectangle(bird.x, bird.y, 40, 40, WHITE);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, LIGHTGRAY);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, BLACK);
                    DrawRectangle(bird.x + 40, bird.y + 20, 10, 10, ORANGE);
                    DrawRectangle(bird.x + 10, bird.y + 25, 20, 5, PINK);
                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, WHITE);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, WHITE);
                    }
                } else if (bird.color == 5){
                    DrawRectangle(bird.x, bird.y, 40, 40, BROWN);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, WHITE);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, BLACK);
                    DrawRectangle(bird.x + 30, bird.y + 20, 30, 10, ORANGE);
                    DrawRectangle(bird.x + 20, bird.y - 5, 10, 5, BROWN);
                    DrawRectangle(bird.x + 10, bird.y - 10, 10, 5, BROWN);
                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, DARKBROWN);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, DARKBROWN);
                    }
                } else if (bird.color == 6){
                    DrawRectangle(bird.x, bird.y, 40, 40, PINK);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, WHITE);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, BLACK);
                    DrawRectangle(bird.x + 30, bird.y + 20, 20, 10, ORANGE);
                    DrawRectangle(bird.x + 35, bird.y + 30, 5, 5, ORANGE);
                    DrawRectangle(bird.x + 40, bird.y + 30, 10, 10, BLACK);
                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, DARKPURPLE);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, DARKPURPLE);
                    }
                } else if (bird.color == 7){
                    DrawRectangle(bird.x, bird.y, 40, 40, PURPLE);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, BLACK);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, WHITE);
                    DrawRectangle(bird.x + 30, bird.y + 20, 20, 10, ORANGE);
                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, DARKBLUE);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, DARKBLUE);
                    }
                } else if (bird.color == 8){
                    DrawRectangle(bird.x, bird.y, 40, 40, BLACK);
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, WHITE);
                    DrawRectangle(bird.x + 20, bird.y + 10, 10, 10, RED);
                    DrawRectangle(bird.x + 30, bird.y + 20, 20, 10, ORANGE);
                    if (bird.flaping) {
                        DrawRectangle(bird.x - 15, bird.y + 20, 30, 20, RED);
                    } else {
                        DrawRectangle(bird.x - 15, bird.y + 10, 30, 20, RED);
                    }
                } else if (bird.color == 9){
                    DrawRectangle(bird.x + 10, bird.y + 10, 20, 20, YELLOW);
                    DrawRectangle(bird.x + 15, bird.y + 15, 10, 10, WHITE);
                    DrawRectangle(bird.x + 20, bird.y + 15, 5, 5, BLACK);
                    DrawRectangle(bird.x + 25, bird.y + 20, 10, 5, ORANGE);
                    if (bird.flaping) {
                        DrawRectangle(bird.x, bird.y + 20, 15, 10, ORANGE);
                    } else {
                        DrawRectangle(bird.x, bird.y + 15, 15, 10, ORANGE);
                    }
                }

                // Desenhar canos com a cor alternada junto ao céu
                for (int i = 0; i < 2; i++) {
                    DrawRectangle(pipe[i].x, 0, 50, pipe[i].height, pipeColor);
                    DrawRectangle(pipe[i].x - 5, pipe[i].height - 15, 60, 15, pipeBorderColor);
                    DrawRectangle(pipe[i].x, pipe[i].height + 150, 50, GetScreenHeight() - pipe[i].height - 150, pipeBorderColor);
                    DrawRectangle(pipe[i].x - 5, pipe[i].height + 150, 60, 15, pipeColor);

                }


                // Mostrar o placar
                DrawText(TextFormat("Score: %d", score), 10, 10, 30, WHITE);

                DrawText(TextFormat("©PietroTy 2024"), 10, 570, 20, WHITE);

                EndDrawing();
            }
    }

    return MENU;
}