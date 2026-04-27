/**
 * @file crappybird.c
 * @brief Crappybird game module — refactored from standalone main.c.
 *
 * Flappy bird clone features:
 *   - Pixel-art bird with multiple unlockable skins
 *   - Pipes with gap, scrolling left
 *   - Day/night color cycle (sky, sun, clouds, pipes)
 *   - Score + highscore tracking
 *   - Store screen to select bird skins (unlocked by highscore)
 *   - Speed increases every 5 points
 *
 * Internal resolution: 600×600 (square)
 * All state is static (isolated from other modules).
 *
 * Original by PietroTy (2024), refactored for Chub hub.
 */
#include <stdio.h>
#include <math.h>

#include "raylib/raylib.h"
#include "chub_colors.h"
#include "game.h"
#include "hub.h"
static int cd_num;
static float cd_timer;

// ══════════════════════════════════════════════
// CONSTANTS
// ══════════════════════════════════════════════

#define CB_WIDTH  600
#define CB_HEIGHT 600

// ══════════════════════════════════════════════
// TYPES
// ══════════════════════════════════════════════

typedef struct {
    float x, y, g;
    bool flaping;
    int color;
} CBBird;

typedef struct {
    int x, height;
    bool isActive;
} CBPipe;

typedef struct {
    float x, y;
} CBCloud;

// ══════════════════════════════════════════════
// STATE (all static)
// ══════════════════════════════════════════════

static CBBird bird;
static CBPipe pipes[2];
static CBCloud clouds[6];
static int score;
static int speed;
static int lastScore;
static int highscore;
static Sound pop_sound;
static bool sound_loaded = false;
static bool gameOver;
static bool store;
static int cb_selection;
static float startTime;
static float cloudSpeed;

// ══════════════════════════════════════════════
// HELPERS
// ══════════════════════════════════════════════

static Color cb_lerp_color(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + t * (b.r - a.r)),
        (unsigned char)(a.g + t * (b.g - a.g)),
        (unsigned char)(a.b + t * (b.b - a.b)),
        255
    };
}

static void cb_reset_game(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    bird.x = (CB_WIDTH - 300) / 2.0f;
    bird.y = CB_HEIGHT / 2.0f;
    bird.g = 0;
    bird.flaping = false;

    for (int i = 0; i < 2; i++) {
        pipes[i].x = 400 + i * 300;
        pipes[i].height = GetRandomValue(100, CB_HEIGHT - 250);
        pipes[i].isActive = true;
    }

    score = 0;
    speed = 2;
    lastScore = 0;
    gameOver = false;
}

static void cb_draw_bird_skin(float bx, float by, int color, bool flap, bool isIcon) {
    Color bCol = (isIcon) ? (Color){45, 45, 45, 255} : BLACK;
    switch (color) {
        case 1:
            DrawRectangle((int)bx, (int)by, 40, 40, YELLOW);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, WHITE);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, bCol);
            DrawRectangle((int)bx + 30, (int)by + 20, 20, 10, ORANGE);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, ORANGE);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, ORANGE);
            break;
        case 2:
            DrawRectangle((int)bx, (int)by, 40, 40, GREEN);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, WHITE);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, bCol);
            DrawRectangle((int)bx + 30, (int)by + 20, 20, 10, ORANGE);
            DrawRectangle((int)bx + 35, (int)by + 30, 5, 5, ORANGE);
            DrawRectangle((int)bx + 40, (int)by + 30, 10, 10, ORANGE);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, DARKGREEN);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, DARKGREEN);
            break;
        case 3:
            DrawRectangle((int)bx, (int)by, 40, 40, DARKBLUE);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, WHITE);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, bCol);
            DrawRectangle((int)bx + 30, (int)by + 20, 20, 10, bCol);
            DrawRectangle((int)bx + 35, (int)by + 30, 5, 5, bCol);
            DrawRectangle((int)bx + 40, (int)by + 30, 10, 10, bCol);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, BLUE);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, BLUE);
            break;
        case 4:
            DrawRectangle((int)bx, (int)by, 40, 40, WHITE);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, LIGHTGRAY);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, bCol);
            DrawRectangle((int)bx + 40, (int)by + 20, 10, 10, ORANGE);
            DrawRectangle((int)bx + 10, (int)by + 25, 20, 5, PINK);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, WHITE);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, WHITE);
            break;
        case 5:
            DrawRectangle((int)bx, (int)by, 40, 40, BROWN);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, WHITE);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, bCol);
            DrawRectangle((int)bx + 30, (int)by + 20, 30, 10, ORANGE);
            DrawRectangle((int)bx + 20, (int)by - 5, 10, 5, BROWN);
            DrawRectangle((int)bx + 10, (int)by - 10, 10, 5, BROWN);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, DARKBROWN);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, DARKBROWN);
            break;
        case 6:
            DrawRectangle((int)bx, (int)by, 40, 40, PINK);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, WHITE);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, bCol);
            DrawRectangle((int)bx + 30, (int)by + 20, 20, 10, ORANGE);
            DrawRectangle((int)bx + 35, (int)by + 30, 5, 5, ORANGE);
            DrawRectangle((int)bx + 40, (int)by + 30, 10, 10, bCol);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, DARKPURPLE);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, DARKPURPLE);
            break;
        case 7:
            DrawRectangle((int)bx, (int)by, 40, 40, PURPLE);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, bCol);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, WHITE);
            DrawRectangle((int)bx + 30, (int)by + 20, 20, 10, ORANGE);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, DARKBLUE);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, DARKBLUE);
            break;
        case 8:
            DrawRectangle((int)bx, (int)by, 40, 40, bCol);
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, WHITE);
            DrawRectangle((int)bx + 20, (int)by + 10, 10, 10, RED);
            DrawRectangle((int)bx + 30, (int)by + 20, 20, 10, ORANGE);
            if (flap) DrawRectangle((int)bx - 15, (int)by + 20, 30, 20, RED);
            else DrawRectangle((int)bx - 15, (int)by + 10, 30, 20, RED);
            break;
        case 9:
            DrawRectangle((int)bx + 10, (int)by + 10, 20, 20, YELLOW);
            DrawRectangle((int)bx + 15, (int)by + 15, 10, 10, WHITE);
            DrawRectangle((int)bx + 20, (int)by + 15, 5, 5, BLACK);
            DrawRectangle((int)bx + 25, (int)by + 20, 10, 5, ORANGE);
            if (flap) DrawRectangle((int)bx, (int)by + 20, 15, 10, ORANGE);
            else DrawRectangle((int)bx, (int)by + 15, 15, 10, ORANGE);
            break;
        default:
            DrawRectangle((int)bx, (int)by, 40, 40, YELLOW);
            break;
    }
}

// ══════════════════════════════════════════════
// LIFECYCLE
// ══════════════════════════════════════════════

void crappybird_init(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    bird.color = 1;
    highscore = hub_load_score(6);
    cb_selection = 0;
    store = false;
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;

    for (int i = 0; i < 6; i++) {
        clouds[i].x = (float)(i * 100);
        clouds[i].y = (float)GetRandomValue(100, CB_HEIGHT);
    }
    cloudSpeed = 1.0f;
    startTime = (float)GetTime();

    cb_reset_game();
    gameOver = false;  // Start immediately playing after global countdown

    printf("[CRAPPYBIRD] Game initialized\n");
}

void crappybird_update(void) {
    if (cd_num > 0) {
        cd_timer += GetFrameTime();
        cd_num = 3 - (int)cd_timer;
        if (cd_num > 0) return;
    }
    if (gameOver) {
        if (store) {
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_ENTER)) store = false;
            
            // Arrow navigation
            if (IsKeyPressed(KEY_RIGHT)) cb_selection = (cb_selection + 1) % 9;
            if (IsKeyPressed(KEY_LEFT)) cb_selection = (cb_selection + 8) % 9;
            if (IsKeyPressed(KEY_DOWN)) cb_selection = (cb_selection + 3) % 9;
            if (IsKeyPressed(KEY_UP)) cb_selection = (cb_selection + 6) % 9;

            // Direct selection
            if (IsKeyPressed(KEY_ONE)) cb_selection = 0;
            if (IsKeyPressed(KEY_TWO)) cb_selection = 1;
            if (IsKeyPressed(KEY_THREE)) cb_selection = 2;
            if (IsKeyPressed(KEY_FOUR)) cb_selection = 3;
            if (IsKeyPressed(KEY_FIVE)) cb_selection = 4;
            if (IsKeyPressed(KEY_SIX)) cb_selection = 5;
            if (IsKeyPressed(KEY_SEVEN)) cb_selection = 6;
            if (IsKeyPressed(KEY_EIGHT)) cb_selection = 7;
            if (IsKeyPressed(KEY_NINE)) cb_selection = 8;

            int thresholds[] = { 0, 15, 20, 25, 30, 35, 40, 45, 50 };
            if (highscore >= thresholds[cb_selection]) {
                bird.color = cb_selection + 1;
            }
        } else {
            if (score > highscore) { highscore = score; hub_save_score(6, highscore); }
            if (IsKeyPressed(KEY_ENTER)) {
                cb_reset_game();
                cd_num = 3;
                cd_timer = 0.0f;
            }
            if (IsKeyPressed(KEY_S)) store = true;
        }
        return;
    }

    // Gravity
    bird.g += 0.5f;
    bird.y += bird.g;

    // Flap
    if (IsKeyPressed(KEY_SPACE)) {
        bird.g = -8.0f;
        if (sound_loaded) PlaySound(pop_sound);
    }
    bird.flaping = (bird.g < 0);

    // Move pipes
    for (int i = 0; i < 2; i++) {
        pipes[i].x -= speed;
        if (pipes[i].x < -50) {
            pipes[i].x = CB_WIDTH;
            pipes[i].height = GetRandomValue(100, CB_HEIGHT - 250);
            pipes[i].isActive = true;
        }
    }

    // Speed increase every 5 points
    if (score >= lastScore + 5 && speed <= 7) {
        speed += 1;
        lastScore = score;
    }

    // Collision with pipes
    for (int i = 0; i < 2; i++) {
        int bw = (bird.color == 9) ? 20 : 40;
        int bh = bw;
        if ((bird.x + bw > pipes[i].x && bird.x < pipes[i].x + 50) &&
            (bird.y < pipes[i].height || bird.y + bh > pipes[i].height + 150)) {
            gameOver = true;
        }
    }

    // Screen bounds
    if (bird.y > CB_HEIGHT || bird.y < 0) gameOver = true;

    // Scoring
    for (int i = 0; i < 2; i++) {
        if (pipes[i].isActive && bird.x > pipes[i].x + 50) {
            score++; if (score > highscore) { highscore = score; hub_save_score(6, highscore); }
            if (sound_loaded) { SetSoundPitch(pop_sound, 1.2f); PlaySound(pop_sound); }
            pipes[i].isActive = false;
        }
    }

    // Move clouds
    for (int i = 0; i < 6; i++) {
        clouds[i].x -= cloudSpeed;
        if (clouds[i].x + 100 < 0) {
            clouds[i].x = (float)CB_WIDTH;
            clouds[i].y = (float)GetRandomValue(100, CB_HEIGHT);
        }
    }
}

void crappybird_draw(void) {
    // ── Store screen ──
    if (gameOver && store) {
        DrawRectangle(0, 0, CB_WIDTH, CB_HEIGHT, BLACK);
        DrawText(L("LOJA", "SHOP"), (CB_WIDTH - MeasureText(L("LOJA", "SHOP"), 60)) / 2, 40, 60, WHITE);
        DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), (CB_WIDTH - MeasureText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), 30)) / 2, 110, 30, GRAY);

        // Draw bird previews in a grid
        int grid_x[] = { 130, 280, 430 };
        int grid_y[] = { 200, 300, 400 };
        int thresholds[] = { 0, 15, 20, 25, 30, 35, 40, 45, 50 };

        for (int i = 0; i < 9; i++) {
            int gx = grid_x[i % 3];
            int gy = grid_y[i / 3];

            // Highlight square in background for selection
            if (cb_selection == i) {
                float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
                DrawRectangle(gx - 10, gy - 10, 60, 60, (Color){200, 200, 200, (unsigned char)(60 + 40 * pulse)}); 
            }


            if (highscore >= thresholds[i]) {
                cb_draw_bird_skin((float)gx, (float)gy, i + 1, false, true);
            } else {
                // Locked
                DrawRectangle(gx, gy, 40, 40, (Color){30, 30, 30, 255});
                DrawText("?", gx + 12, gy + 5, 30, GRAY);
                DrawText(TextFormat(L("%d pts", "%d pts"), thresholds[i]), gx - 10, gy + 45, 12, RED);
            }
            DrawText(TextFormat("%d", i + 1), gx - 35, gy + 10, 20, WHITE);
        }

        DrawText(L("SETAS para navegar | ENTER para selecionar e sair", "ARROWS to navigate | ENTER to select and exit"), (CB_WIDTH - MeasureText(L("SETAS para navegar | ENTER para selecionar e sair", "ARROWS to navigate | ENTER to select and exit"), 20)) / 2, CB_HEIGHT - 45, 20, GRAY);

        return;
    }

    // ── Game over screen ──
    if (gameOver) {
        DrawRectangle(0, 0, CB_WIDTH, CB_HEIGHT, (Color){0, 0, 0, 180});
        
        int center_y = CB_HEIGHT / 2 - 80;
        const char *title = L("FIM DE JOGO", "GAME OVER");
        Color title_col = RED;
        int tw = MeasureText(title, 60);
        DrawText(title, (CB_WIDTH - tw) / 2, center_y, 60, title_col);
        
        DrawText(TextFormat(L("Pontos: %d", "Score: %d"), score), (CB_WIDTH - MeasureText(TextFormat(L("Pontos: %d", "Score: %d"), score), 40)) / 2, center_y + 80, 40, WHITE);
        DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), (CB_WIDTH - MeasureText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), 24)) / 2, center_y + 130, 24, GRAY);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        Color pulse_c = (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)};
        DrawText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), (CB_WIDTH - MeasureText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), 20)) / 2, center_y + 190, 20, pulse_c);
        DrawText(L("Pressione S para Loja", "Press S for Shop"), (CB_WIDTH - MeasureText(L("Pressione S para Loja", "Press S for Shop"), 20)) / 2, center_y + 220, 20, pulse_c);
        
        return;
    }

    // ── Active game ──
    float elapsedTime = (float)GetTime() - startTime;
    float cycleTime = 40.0f;
    float t = fmodf(elapsedTime, cycleTime) / cycleTime;

    Color bgCol, sunCol, cloudCol, pipeCol, pipeBorderCol;
    if (t < 0.5f) {
        bgCol = cb_lerp_color(SKYBLUE, DEEPBLUE, t * 2);
        sunCol = cb_lerp_color(GOLD, DARKORANGE, t * 2);
        cloudCol = cb_lerp_color(WHITE, DARKGRAY, t * 2);
        pipeCol = cb_lerp_color(LIME, DARKGREEN, t * 2);
        pipeBorderCol = cb_lerp_color(DARKGREEN, LIME, t * 2);
    } else {
        bgCol = cb_lerp_color(DEEPBLUE, SKYBLUE, (t - 0.5f) * 2);
        sunCol = cb_lerp_color(DARKORANGE, GOLD, (t - 0.5f) * 2);
        cloudCol = cb_lerp_color(DARKGRAY, WHITE, (t - 0.5f) * 2);
        pipeCol = cb_lerp_color(DARKGREEN, LIME, (t - 0.5f) * 2);
        pipeBorderCol = cb_lerp_color(LIME, DARKGREEN, (t - 0.5f) * 2);
    }

    ClearBackground(bgCol);

    // Sun
    DrawRectangle((int)clouds[5].x, 100, 100, 100, sunCol);

    // Clouds
    for (int i = 0; i < 6; i++) {
        DrawRectangle((int)clouds[i].x, (int)clouds[i].y, 100, 20, cloudCol);
        DrawRectangle((int)clouds[i].x + 30, (int)clouds[i].y - 20, 40, 20, cloudCol);
    }

    // Bird
    cb_draw_bird_skin(bird.x, bird.y, bird.color, bird.flaping, false);

    // Pipes
    for (int i = 0; i < 2; i++) {
        DrawRectangle(pipes[i].x, 0, 50, pipes[i].height, pipeCol);
        DrawRectangle(pipes[i].x - 5, pipes[i].height - 15, 60, 15, pipeBorderCol);
        DrawRectangle(pipes[i].x, pipes[i].height + 150, 50, CB_HEIGHT - pipes[i].height - 150, pipeBorderCol);
        DrawRectangle(pipes[i].x - 5, pipes[i].height + 150, 60, 15, pipeCol);
    }

    // HUD
    DrawText(TextFormat(L("Pontos: %d", "Score: %d"), score), 10, 10, 30, WHITE);
    const char *hs_text = TextFormat(L("Recorde: %d", "Highscore: %d"), highscore);
    DrawText(hs_text, CB_WIDTH - MeasureText(hs_text, 30) - 10, 10, 30, WHITE);




    if (cd_num > 0) {
        char cd_text[16];
        snprintf(cd_text, sizeof(cd_text), "%d", cd_num);
        int cd_size = 100;
        float frac = cd_timer - (int)cd_timer;
        unsigned char alpha = (unsigned char)(255 * (1.0f - frac));
        float scale = 1.0f + frac * 0.5f;
        int scaled_size = (int)(cd_size * scale);
        int scaled_w = MeasureText(cd_text, scaled_size);
        Color cd_col = (Color){ 255, 255, 200, alpha };
        DrawText(cd_text, (CB_WIDTH - scaled_w) / 2, (CB_HEIGHT - scaled_size) / 2, scaled_size, cd_col);
    }
}

void crappybird_unload(void) {
    if (sound_loaded) UnloadSound(pop_sound);
    printf("[CRAPPYBIRD] Game unloaded\n");
}

// ══════════════════════════════════════════════
// GAME MODULE DEFINITION
// ══════════════════════════════════════════════

Game CRAPPYBIRD_GAME = {
    .name        = "Crappy bird",
    .description = "Voe entre canos e desbloqueie skins",
    .description_en = "Fly between pipes and unlock skins",
    .game_width  = CB_WIDTH,
    .game_height = CB_HEIGHT,
    .init        = crappybird_init,
    .update      = crappybird_update,
    .draw        = crappybird_draw,
    .unload      = crappybird_unload,
};
