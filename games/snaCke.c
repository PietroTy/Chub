#include "snaCke.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <raylib.h>

typedef struct Snake {
    float x;
    float y;
} Snake;

typedef struct Apple {
    int x;
    int y;
} Apple;

typedef struct Grape {
    int x;
    int y;
    bool isVisible;
    float spawnTime;
} Grape;

typedef struct Orange {
    int x;
    int y;
    bool isVisible;
    float spawnTime;
} Orange;

// Função para desenhar os segmentos conectados
void DrawSnakeSegment(Snake segment, int segmentWidth, int segmentHeight, int segN, bool isHead, bool drawLeft, bool drawRight, bool drawUp, bool drawDown) {
    Color segmentColor = (segN % 2 == 0) ? DARKGREEN : LIME;

    if (isHead) {
        // Desenhar a cabeça da cobra
        DrawRectangle(segment.x + 5, segment.y + 5, segmentWidth, segmentHeight, GREEN);  // Cor da cabeça
        
        // Conectar a cabeça ao primeiro segmento do corpo, se existir
        if (drawLeft) {
            DrawRectangle(segment.x - 5, segment.y + 5, 10, segmentHeight, GREEN);
        }
        if (drawRight) {
            DrawRectangle(segment.x + segmentWidth + 5, segment.y + 5, 10, segmentHeight, GREEN);
        }
        if (drawUp) {
            DrawRectangle(segment.x + 5, segment.y - 5, segmentWidth, 10, GREEN);
        }
        if (drawDown) {
            DrawRectangle(segment.x + 5, segment.y + segmentHeight + 5, segmentWidth, 10, GREEN);
        }
    
    } else {
        // Desenha o corpo principal
        DrawRectangle(segment.x + 5, segment.y + 5, segmentWidth, segmentHeight, segmentColor);

        // Desenha os conectores entre os segmentos
        if (drawLeft) {
            DrawRectangle(segment.x - 5, segment.y + 5, 10, segmentHeight, segmentColor);
        }
        if (drawRight) {
            DrawRectangle(segment.x + segmentWidth + 5, segment.y + 5, 10, segmentHeight, segmentColor);
        }
        if (drawUp) {
            DrawRectangle(segment.x + 5, segment.y - 5, segmentWidth, 10, segmentColor);
        }
        if (drawDown) {
            DrawRectangle(segment.x + 5, segment.y + segmentHeight + 5, segmentWidth, 10, segmentColor);
        }
    }
}

// Função para verificar se uma fruta está sobre o corpo da cobra
bool IsPositionOccupiedBySnake(int x, int y, Snake head, Snake *body, int bodyLength) {
    // Verifica se está na posição da cabeça
    if (x == head.x && y == head.y) {
        return true;
    }
    // Verifica se está em alguma parte do corpo
    for (int i = 0; i < bodyLength; i++) {
        if (x == body[i].x && y == body[i].y) {
            return true;
        }
    }
    return false;
}

// Função para verificar se uma posição está ocupada por alguma fruta
bool IsPositionOccupiedByFruit(int x, int y, Apple apple, Grape grape, Orange orange) {
    // Verifica a maçã
    if (x == apple.x && y == apple.y) return true;
    // Verifica a uva
    if (grape.isVisible && x == grape.x && y == grape.y) return true;
    // Verifica a laranja
    if (orange.isVisible && x == orange.x && y == orange.y) return true;
    
    return false;
}


// Função para gerar uma nova posição válida para a maçã ou uva
void GenerateValidPositionForFruit(Apple *apple, Grape *grape, Orange *orange, Snake head, Snake *body, int bodyLength, bool isApple, bool isGrape) {
    int fruitX, fruitY;
    do {
        fruitX = GetRandomValue(1, 11) * 50;
        fruitY = GetRandomValue(1, 11) * 50;
    } while (IsPositionOccupiedBySnake(fruitX, fruitY, head, body, bodyLength));

    if (isApple && apple != NULL) {
        apple->x = fruitX;
        apple->y = fruitY;
    } else if (isGrape && grape != NULL) {
        grape->x = fruitX;
        grape->y = fruitY;
    } else if (orange != NULL) {
        orange->x = fruitX;
        orange->y = fruitY;
    }
}


static void ResetGame(Snake *head, Snake *body, int *bodyLength, Apple *apple, Grape *grape, Orange *orange, int *score, float *moveInterval, bool *gameOver, bool *moved, float *lastGrapeAppearanceTime, float *lastOrangeAppearanceTime, float currentTime, int *direction, bool *victory) {
    // Reiniciar cobra
    head->x = GetRandomValue(1, 11) * 50;  // Garante que a cobra começa longe das bordas
    head->y = GetRandomValue(1, 11) * 50;
    *bodyLength = 2;

    // Reiniciar corpo (coloca o corpo em uma posição válida atrás da cabeça)
    for (int i = 0; i < *bodyLength; i++) {
        body[i].x = head->x - ((i + 1) * 50);
        body[i].y = head->y;
    }

    // Reiniciar maçã e uva
    GenerateValidPositionForFruit(apple, NULL, NULL, *head, body, *bodyLength, true, false);
    grape->isVisible = false;
    orange->isVisible = false;
    *lastGrapeAppearanceTime = currentTime;
    *lastOrangeAppearanceTime = currentTime;

    // Resetar outras variáveis
    *score = 0;
    *gameOver = false;
    *victory = false;
    *moved = false;
    *moveInterval = 0.2f;
    *direction = 0;
    
}
// Função para salvar o *highscore* em um arquivo
static void SaveHighscore(int highscore) {
    FILE *file = fopen("resources/SnackeHighscore.txt", "w");
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
    FILE *file = fopen("resources/SnackeHighscore.txt", "r");
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

GameState snaCke() {
    // Game logic for snaCke here
    // Example placeholder logic:
    while (1) {
        if (IsKeyPressed(KEY_ESCAPE)) return MENU;  // Exit to menu

            const int segmentWidth = 40;
            const int segmentHeight = 40;
            Snake head;
            Apple apple;
            Grape grape;
            Orange orange;
            Snake body[144]; 
            int lastDirection = 0;
            int highscore = LoadHighscore();
            int bodyLength = 2;
            int direction = 0;
            int score = 0;
            bool gameOver = false, victory = false, moved = false, open = false;
            float moveInterval = 0.2f, lastMoveTime = 0.0f, lastGrapeAppearanceTime = 0.0f, lastOrangeAppearanceTime = 0.0f;

            SetConfigFlags(FLAG_MSAA_4X_HINT);
            SetWindowIcon(LoadImage("resources/iconeSnaCke.png"));
            SetTargetFPS(60);

            InitAudioDevice();

            Sound pop = LoadSound("resources/Pop.wav");

            ResetGame(&head, body, &bodyLength, &apple, &grape, &orange, &score, &moveInterval, &gameOver, &moved, &lastGrapeAppearanceTime, &lastOrangeAppearanceTime, (float) GetTime(), &direction, &victory);

            while (!WindowShouldClose()) {
                float currentTime = GetTime(); // Obter o tempo atual
                float deltaTime = currentTime - lastMoveTime;

                // Aumentar velocidade da cobra a cada 20 pontos
                if (score % 20 == 0 && score != 0) {
                    moveInterval = 0.2f - (0.01f * (score / 20));
                }

                if (gameOver) {

                    if (score > highscore) {
                        highscore = score;
                        SaveHighscore(highscore); // Salva o novo *highscore*
                    }

                    if (IsKeyPressed(KEY_R)) {
                        ResetGame(&head, body, &bodyLength, &apple, &grape, &orange, &score, &moveInterval, &gameOver, &moved, &lastGrapeAppearanceTime, &lastOrangeAppearanceTime, (float) GetTime(), &direction, &victory);
                    }

                    BeginDrawing();
                    ClearBackground(BLACK);
                    const char *lostMessage = "You Lost!";
                    const char *scoreMessage = TextFormat("Score: %d", score);
                    const char *highscoreMessage = TextFormat("Highscore: %d", highscore);
                    const char *resetMessage = "press R to reset";
                    DrawText(lostMessage, (GetScreenWidth() - MeasureText(lostMessage, 100)) / 2, (GetScreenHeight() - 200) / 2, 100, WHITE);
                    DrawText(scoreMessage, (GetScreenWidth() - MeasureText(scoreMessage, 60)) / 2, (GetScreenHeight() + 100) / 2, 60, WHITE);
                    DrawText(highscoreMessage, (GetScreenWidth() - MeasureText(highscoreMessage, 40)) / 2, (GetScreenHeight() + 260) / 2, 40, WHITE);
                    DrawText(resetMessage, (GetScreenWidth() - MeasureText(resetMessage, 30)) / 2, (GetScreenHeight() + 390) / 2, 30, WHITE);
                    
                    DrawText(TextFormat("©PietroTy 2024"), 10, 570, 20, WHITE);

                    EndDrawing();

                    continue;
                }

                if (victory) {

                    if (score > highscore) {
                        highscore = score;
                        SaveHighscore(highscore); // Salva o novo *highscore*
                    }
                    
                    if (IsKeyPressed(KEY_R)) {
                        ResetGame(&head, body, &bodyLength, &apple, &grape, &orange, &score, &moveInterval, &gameOver, &moved, &lastGrapeAppearanceTime, &lastOrangeAppearanceTime, (float) GetTime(), &direction, &victory);
                    }

                    BeginDrawing();
                    ClearBackground(WHITE);
                    const char *winMessage = "You Won!";
                    const char *scoreMessage = TextFormat("Score: %d", score);
                    const char *resetMessage = "press R to reset";
                    DrawText(winMessage, (GetScreenWidth() - MeasureText(winMessage, 100)) / 2, (GetScreenHeight() - 200) / 2, 100, BLACK);
                    DrawText(scoreMessage, (GetScreenWidth() - MeasureText(scoreMessage, 60)) / 2, (GetScreenHeight() + 100) / 2, 60, BLACK);
                    DrawText(resetMessage, (GetScreenWidth() - MeasureText(resetMessage, 30)) / 2, (GetScreenHeight() + 300) / 2, 30, BLACK);

                    DrawText(TextFormat("©PietroTy 2024"), 10, 570, 20, WHITE);

                    EndDrawing();
                    continue;
                }
                



                // Controle de movimento
                if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_W) ||
                    IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP)) {
                    
                    int newDirection = direction; // Variável temporária para a nova direção
                    
                    // Atualizar nova direção apenas se não for oposta à atual
                    if ((IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) && lastDirection != 2) newDirection = 1; // Direita
                    else if ((IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) && lastDirection != 1) newDirection = 2; // Esquerda
                    else if ((IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) && lastDirection != 4) newDirection = 3; // Baixo
                    else if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && lastDirection != 3) newDirection = 4; // Cima
                    
                    // Atualiza a direção apenas se houver uma mudança válida
                    if (newDirection != direction) {
                        direction = newDirection;
                        moved = true;
                    }
                }

                // Movimento da cobra
                if (deltaTime >= moveInterval) {
                    if(open) open = false;
                    else open = true;
                    lastMoveTime = currentTime;
                    lastDirection = direction; // Armazena a direção atual antes de atualizar a posição

                    // Resto do código de movimento da cobra
                    for (int i = bodyLength - 1; i > 0; i--) body[i] = body[i - 1];
                    if (bodyLength > 0) {
                        body[0].x = head.x;
                        body[0].y = head.y;
                    }
                    if (direction == 1) head.x += 50;
                    else if (direction == 2) head.x -= 50;
                    else if (direction == 3) head.y += 50;
                    else if (direction == 4) head.y -= 50;
                }


                // Colisão com o corpo
                if (moved && bodyLength > 2) {
                    for (int i = 0; i < bodyLength; i++) {
                        if (head.x == body[i].x && head.y == body[i].y) gameOver = true;
                    }
                }

                // Lógica de spawn da uva
                if (grape.isVisible && currentTime - grape.spawnTime >= 3.0f) {
                    grape.isVisible = false;
                    lastGrapeAppearanceTime = currentTime;
                } else if (!grape.isVisible && currentTime - lastGrapeAppearanceTime >= 12.0f) {
                    GenerateValidPositionForFruit(NULL, &grape, NULL, head, body, bodyLength, false, true);
                    grape.isVisible = true;
                    grape.spawnTime = currentTime;
                }

                // Lógica de spawn da laranja
                if (orange.isVisible && currentTime - orange.spawnTime >= 15.0f) {
                    orange.isVisible = false;
                    lastOrangeAppearanceTime = currentTime;
                } else if (!orange.isVisible && currentTime - lastOrangeAppearanceTime >= 30.0f) {
                    GenerateValidPositionForFruit(NULL, NULL, &orange, head, body, bodyLength, false, false);
                    orange.isVisible = true;
                    orange.spawnTime = currentTime;
                }


                // Verificar se comeu a maçã
                if (head.x == apple.x && head.y == apple.y) {
                    score++;
                    PlaySound(pop);
                    body[bodyLength] = body[bodyLength - 1]; // Cresce o corpo
                    bodyLength++;
                    GenerateValidPositionForFruit(&apple, NULL, NULL, head, body, bodyLength, true, false);
                }

                // Verificar se comeu a uva
                if (grape.isVisible && head.x == grape.x && head.y == grape.y) {
                    score += 3;
                    PlaySound(pop);
                    for (int i = 0; i < 3; i++) body[bodyLength++] = body[bodyLength - 1];
                    grape.isVisible = false;
                    lastGrapeAppearanceTime = currentTime;
                }

                // Verificar se comeu a laranja
                if (orange.isVisible && head.x == orange.x && head.y == orange.y) {
                    score += 10;
                    PlaySound(pop);
                    int currentLength = bodyLength;
                    for (int i = 0; i < 10; i++) {body[bodyLength++] = body[currentLength - 1];}
                    orange.isVisible = false;
                    lastOrangeAppearanceTime = currentTime;
                }



                // Verificar limites da tela
                if (head.x < 0 || head.x >= GetScreenWidth() || head.y < 0 || head.y >= GetScreenHeight()) gameOver = true;

                // Desenhar
                BeginDrawing();
                ClearBackground(BLACK);

                // Desenhar grid
                for (int i = 0; i <= GetScreenWidth(); i += 100) {
                    for (int j = 0; j <= GetScreenHeight(); j += 100) DrawRectangle(i, j, 50, 50, THEGRAY);
                    for (int j = 0; j <= GetScreenHeight(); j += 100) DrawRectangle(i + 50, j + 50, 50, 50, THEGRAY);
                }

                // Desenhar a cabeça da cobra
                bool headLeft = (bodyLength > 0 && head.x > body[0].x);
                bool headRight = (bodyLength > 0 && head.x < body[0].x);
                bool headUp = (bodyLength > 0 && head.y > body[0].y);
                bool headDown = (bodyLength > 0 && head.y < body[0].y);
                DrawSnakeSegment(head, segmentWidth, segmentHeight, 0, true, headLeft, headRight, headUp, headDown);
                if (open) {
                    DrawRectangle(head.x + 15, head.y + 15, 20, 20, BLACK);
                } else {
                    DrawRectangle(head.x + 10, head.y + 20, 30, 10, BLACK);
                }

                // Desenhar o corpo da cobra
                for (int i = 0; i < bodyLength; i++) {
                    bool drawLeft = (i > 0 && body[i].x > body[i - 1].x);
                    bool drawRight = (i > 0 && body[i].x < body[i - 1].x);
                    bool drawUp = (i > 0 && body[i].y > body[i - 1].y);
                    bool drawDown = (i > 0 && body[i].y < body[i - 1].y);

                    DrawSnakeSegment(body[i], segmentWidth, segmentHeight, i + 1, false, drawLeft, drawRight, drawUp, drawDown);
                }

                // Desenhar maçã
                DrawRectangle(apple.x + 5, apple.y + 5, 40, 40, RED);
                DrawRectangle(apple.x + 20, apple.y, 20, 10, LIME);

                // Desenhar uva
                if (grape.isVisible) {
                    DrawRectangle(grape.x + 5, grape.y + 5, 18, 18, PURPLE);
                    DrawRectangle(grape.x + 27, grape.y + 5, 18, 18, PURPLE);
                    DrawRectangle(grape.x + 16, grape.y + 28, 18, 18, PURPLE);
                    DrawRectangle(grape.x + 20, grape.y, 20, 10, LIME);
                }

                if (orange.isVisible) {
                    DrawRectangle(orange.x + 5, orange.y + 5, 40, 40, ORANGE);
                    DrawRectangle(orange.x + 20, orange.y, 20, 10, LIME);
                }

                // Desenhar HUD do placar
                DrawText(TextFormat("Score: %d", score), 10, 10, 30, WHITE);

                // Verificar condição de vitória
                if (bodyLength >= 100) {
                    victory = true;
                }

                DrawText(TextFormat("©PietroTy 2024"), 10, 570, 20, WHITE);

                EndDrawing();
            }
    }

    CloseAudioDevice();


    return MENU;
}