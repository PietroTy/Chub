/**
 * @file tectris.c
 * @brief Tetris game module — refactored from standalone teCtris main.c.
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "raylib/raylib.h"
#include "hub.h"
#include <math.h>
#include "chub_colors.h"
#include "game.h"

static int cd_num;
static float cd_timer;

// ══════════════════════════════════════════════
// CONSTANTS
// ══════════════════════════════════════════════

#define TC_GRID_W   10
#define TC_GRID_H   20
#define TC_BLOCK    30
#define TC_WIDTH    450   // Expanded from 300 to fit side panels
#define TC_HEIGHT   600   // 20 blocks high
#define TC_PLAYFIELD_W 300 
#define TC_SIDE_W   150

// ══════════════════════════════════════════════
// TYPES
// ══════════════════════════════════════════════

typedef struct {
    int x, y;
    int shape[4][4];
    int rotation;
    int colorIndex;
} TCTetromino;

// ══════════════════════════════════════════════
// TETROMINO SHAPES
// ══════════════════════════════════════════════

static const int TC_SHAPES[8][4][4] = {
    // I
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    // O
    {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    // T
    {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    // L
    {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    // J
    {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    // S
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    // Z
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    // Q (rare)
    {{1,1,1,0},{1,0,1,0},{1,1,1,0},{0,1,0,0}},
};

// ══════════════════════════════════════════════
// STATE (all static)
// ══════════════════════════════════════════════

static int grid[TC_GRID_H][TC_GRID_W];
static TCTetromino current;
static TCTetromino next_piece;
static TCTetromino hold_piece;
static bool has_hold = false;
static bool can_hold = true;

static int tc_score;
static int tc_highscore;
static bool tc_gameOver;
static float tc_fallInterval;
static float tc_lastFallTime;
static bool tc_seeded;
static Sound pop_sound;
static bool sound_loaded = false;

static Color colorsOut[8];
static Color colorsIn[8];

// ══════════════════════════════════════════════
// HELPERS
// ══════════════════════════════════════════════

static void tc_init_colors(void) {
    colorsOut[0] = DARKBLUE;
    colorsOut[1] = DARKGREEN;
    colorsOut[2] = DARKPURPLE;
    colorsOut[3] = DARKYELLOW;
    colorsOut[4] = DARKSKY;
    colorsOut[5] = DARKORANGE;
    colorsOut[6] = DARKRED;
    colorsOut[7] = LIGHTGRAY;

    colorsIn[0] = BLUE;
    colorsIn[1] = GREEN;
    colorsIn[2] = PURPLE;
    colorsIn[3] = YELLOW;
    colorsIn[4] = SKYBLUE;
    colorsIn[5] = ORANGE;
    colorsIn[6] = RED;
    colorsIn[7] = WHITE;
}

static bool tc_collision(void) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current.shape[i][j]) {
                int x = current.x + j;
                int y = current.y + i;
                if (x < 0 || x >= TC_GRID_W || y >= TC_GRID_H || (y >= 0 && grid[y][x]))
                    return true;
            }
        }
    }
    return false;
}

static bool tc_collision_at(int yOffset) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current.shape[i][j]) {
                int x = current.x + j;
                int y = yOffset + i;
                if (x < 0 || x >= TC_GRID_W || y >= TC_GRID_H || (y >= 0 && grid[y][x]))
                    return true;
            }
        }
    }
    return false;
}

static void tc_draw_piece_centered(TCTetromino p, int bx, int by, int bw, int bh, int size) {
    int minR = 4, maxR = -1, minC = 4, maxC = -1;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (p.shape[i][j]) {
                if (i < minR) minR = i; 
                if (i > maxR) maxR = i;
                if (j < minC) minC = j; 
                if (j > maxC) maxC = j;
            }
        }
    }
    if (maxR == -1) return; 
    int pW = (maxC - minC + 1) * size;
    int pH = (maxR - minR + 1) * size;
    int ox = bx + (bw - pW) / 2 - minC * size;
    int oy = by + (bh - pH) / 2 - minR * size;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (p.shape[i][j]) {
                DrawRectangle(ox + j * size, oy + i * size, size, size, colorsOut[p.colorIndex]);
                DrawRectangle(ox + j * size + 4, oy + i * size + 4, size - 8, size - 8, colorsIn[p.colorIndex]);
            }
        }
    }
}

static int tc_get_random_piece_index(void) {
    static const int probs[] = {
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,
        3,3,3,3,3,3,3,3,
        4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,
        7
    };
    return probs[rand() % (sizeof(probs)/sizeof(probs[0]))];
}

static void tc_init_piece(TCTetromino *p, int idx) {
    p->x = TC_GRID_W / 2 - 2;
    p->y = 0;
    p->rotation = 0;
    p->colorIndex = idx;
    memcpy(p->shape, TC_SHAPES[idx], sizeof(p->shape));
}

static void tc_spawn_piece(void) {
    current = next_piece;
    tc_init_piece(&next_piece, tc_get_random_piece_index());
    can_hold = true;
    if (tc_collision()) tc_gameOver = true;
}

static void tc_hold_piece(void) {
    if (!can_hold) return;
    can_hold = false;

    // Reset rotation before storing
    current.rotation = 0;
    memcpy(current.shape, TC_SHAPES[current.colorIndex], sizeof(current.shape));
    
    if (!has_hold) {
        hold_piece = current;
        has_hold = true;
        tc_spawn_piece();
    } else {
        TCTetromino temp = current;
        current = hold_piece;
        hold_piece = temp;
        current.x = TC_GRID_W / 2 - 2;
        current.y = 0;
    }
}

static void tc_lock(void) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current.shape[i][j]) {
                int x = current.x + j;
                int y = current.y + i;
                if (y >= 0 && x >= 0 && x < TC_GRID_W) grid[y][x] = current.colorIndex + 1;
            }
        }
    }
}

static void tc_clear_lines(void) {
    int cleared = 0;
    for (int i = TC_GRID_H - 1; i >= 0; i--) {
        bool full = true;
        for (int j = 0; j < TC_GRID_W; j++) {
            if (!grid[i][j]) { full = false; break; }
        }
        if (full) {
            cleared++;
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < TC_GRID_W; j++) grid[k][j] = grid[k-1][j];
            }
            for (int j = 0; j < TC_GRID_W; j++) grid[0][j] = 0;
            i++; 
        }
    }
    if (cleared > 0) {
        if (sound_loaded) { SetSoundPitch(pop_sound, 1.2f + cleared * 0.1f); PlaySound(pop_sound); }
        if (cleared == 1) tc_score += 1;
        else if (cleared == 2) tc_score += 3;
        else if (cleared == 3) tc_score += 5;
        else if (cleared >= 4) tc_score += 10;
        if (tc_score > tc_highscore) { tc_highscore = tc_score; hub_save_score(2, tc_highscore); }
    }
}

static void tc_rotate(void) {
    if (current.colorIndex == 1) return;
    if (current.colorIndex == 0) {
        if (current.rotation % 2 == 0) {
            current.shape[0][1]=1; current.shape[1][1]=1; current.shape[2][1]=1; current.shape[3][1]=1;
            current.shape[1][0]=0; current.shape[1][2]=0; current.shape[1][3]=0;
        } else {
            current.shape[0][1]=0; current.shape[2][1]=0; current.shape[3][1]=0;
            current.shape[1][0]=1; current.shape[1][1]=1; current.shape[1][2]=1; current.shape[1][3]=1;
        }
        current.rotation = (current.rotation + 1) % 2;
        return;
    }
    if (current.colorIndex == 7) {
        int temp[4][4] = {{0}};
        for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) temp[j][3-i] = current.shape[i][j];
        memcpy(current.shape, temp, sizeof(temp));
        return;
    }
    int temp[4][4] = {{0}};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current.shape[i][j]) {
                int ni = j - 1 + 1;
                int nj = 1 - i + 1;
                if (ni >= 0 && ni < 4 && nj >= 0 && nj < 4) temp[ni][nj] = current.shape[i][j];
            }
        }
    }
    memcpy(current.shape, temp, sizeof(temp));
}

static void tc_rotate_back(void) {
    for (int i = 0; i < 3; i++) tc_rotate();
}

static void tc_reset_game(void) {
    tc_score = 0;
    tc_gameOver = false;
    memset(grid, 0, sizeof(grid));
    has_hold = false;
    can_hold = true;
    tc_init_piece(&next_piece, tc_get_random_piece_index());
    tc_spawn_piece();
    tc_fallInterval = 0.5f;
    tc_lastFallTime = (float)GetTime();
}

void tectris_init(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    tc_init_colors();
    tc_highscore = hub_load_score(2);
    if (!tc_seeded) { srand((unsigned int)time(NULL)); tc_seeded = true; }
    tc_reset_game();
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;
    printf("[TECTRIS] Game initialized\n");
}

void tectris_update(void) {
    if (cd_num > 0) {
        cd_timer += GetFrameTime();
        cd_num = 3 - (int)cd_timer;
        if (cd_num > 0) return;
    }
    if (tc_gameOver) {
        if (IsKeyPressed(KEY_ENTER)) { tc_reset_game(); cd_num = 3; cd_timer = 0.0f; }
        return;
    }

    float currentTime = (float)GetTime();
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) tc_fallInterval = 0.05f;
    else tc_fallInterval = 0.5f;

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) { current.x -= 1; if (tc_collision()) current.x += 1; }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { current.x += 1; if (tc_collision()) current.x -= 1; }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { tc_rotate(); if (tc_collision()) tc_rotate_back(); }
    if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_LEFT_SHIFT)) { tc_hold_piece(); }

    if (currentTime - tc_lastFallTime >= tc_fallInterval) {
        tc_lastFallTime = currentTime;
        current.y += 1;
        if (tc_collision()) {
            current.y -= 1;
            tc_lock();
            if (sound_loaded) { SetSoundPitch(pop_sound, 0.9f); PlaySound(pop_sound); }
            tc_clear_lines();
            tc_spawn_piece();
            if (tc_collision()) tc_gameOver = true;
        }
    }
}

void tectris_draw(void) {
    ClearBackground(BLACK);

    // Background stripes (playfield only)
    for (int i = 0; i <= TC_HEIGHT; i += TC_BLOCK * 2) DrawRectangle(0, i, TC_PLAYFIELD_W, TC_BLOCK, (Color){20, 20, 20, 255});
    DrawLine(TC_PLAYFIELD_W, 0, TC_PLAYFIELD_W, TC_HEIGHT, GRAY);

    // ── Sidebar (Right Panel) ──
    int sb_x = TC_PLAYFIELD_W + 10;
    
    // Hold Piece
    DrawText(L("BANCO", "HOLD"), sb_x + 5, 20, 20, LIGHTGRAY);
    DrawRectangleLines(sb_x, 50, 130, 110, GRAY);
    if (has_hold) {
        tc_draw_piece_centered(hold_piece, sb_x, 50, 130, 110, 25);
    }

    // Next Piece
    DrawText(L("PROXIMA", "NEXT"), sb_x + 5, 180, 20, LIGHTGRAY);
    DrawRectangleLines(sb_x, 210, 130, 110, GRAY);
    tc_draw_piece_centered(next_piece, sb_x, 210, 130, 110, 25);

    // Score & Highscore
    DrawText(L("PONTOS", "SCORE"), sb_x + 5, 350, 20, LIGHTGRAY);
    DrawText(TextFormat("%d", tc_score), sb_x + 5, 380, 25, WHITE);
    
    DrawText(L("RECORDE", "HIGH"), sb_x + 5, 430, 20, LIGHTGRAY);
    DrawText(TextFormat("%d", tc_highscore), sb_x + 5, 460, 25, WHITE);

    // Localized controls hint
    DrawText(L("Setas/WASD: Mover", "Arrows/WASD: Move"), sb_x + 5, 520, 12, (Color){150, 150, 150, 120});
    DrawText(L("C/SHIFT: Banco", "C/SHIFT: Hold"), sb_x + 5, 540, 12, (Color){150, 150, 150, 120});

    // ── Playfield Content ──
    for (int i = 0; i < TC_GRID_H; i++) {
        for (int j = 0; j < TC_GRID_W; j++) {
            if (grid[i][j]) {
                DrawRectangle(j * TC_BLOCK, i * TC_BLOCK, TC_BLOCK, TC_BLOCK, colorsOut[grid[i][j] - 1]);
                DrawRectangle(j * TC_BLOCK + 5, i * TC_BLOCK + 5, 20, 20, colorsIn[grid[i][j] - 1]);
            }
        }
    }

    if (!tc_gameOver) {
        int previewY = current.y;
        while (!tc_collision_at(previewY + 1)) previewY++;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (current.shape[i][j]) {
                    int x = (current.x + j) * TC_BLOCK;
                    int y = (previewY + i) * TC_BLOCK;
                    DrawRectangle(x, y, TC_BLOCK, TC_BLOCK, (Color){40, 40, 40, 255});
                    DrawRectangle(x + 5, y + 5, 20, 20, (Color){60, 60, 60, 255});
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (current.shape[i][j]) {
                    int x = (current.x + j) * TC_BLOCK;
                    int y = (current.y + i) * TC_BLOCK;
                    DrawRectangle(x, y, TC_BLOCK, TC_BLOCK, colorsOut[current.colorIndex]);
                    DrawRectangle(x + 5, y + 5, 20, 20, colorsIn[current.colorIndex]);
                }
            }
        }
    }

    // ── Game Over Overlay ──
    if (tc_gameOver) {
        DrawRectangle(0, 0, TC_WIDTH, TC_HEIGHT, (Color){0, 0, 0, 200});
        int center_y = TC_HEIGHT / 2 - 80;
        const char *lost = L("FIM DE JOGO", "GAME OVER");
        DrawText(lost, (TC_WIDTH - MeasureText(lost, 40)) / 2, center_y, 40, RED);
        DrawText(TextFormat(L("Pontos: %d", "Score: %d"), tc_score), (TC_WIDTH - MeasureText(TextFormat(L("Pontos: %d", "Score: %d"), tc_score), 30)) / 2, center_y + 60, 30, WHITE);
        DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), tc_highscore), (TC_WIDTH - MeasureText(TextFormat(L("Recorde: %d", "Highscore: %d"), tc_highscore), 20)) / 2, center_y + 100, 20, GRAY);
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        const char *hint = L("Pressione ENTER para Reiniciar", "Press ENTER to Restart");
        DrawText(hint, (TC_WIDTH - MeasureText(hint, 16)) / 2, center_y + 150, 16, (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)});
    }

    if (cd_num > 0 && !tc_gameOver) {
        char cd_text[16]; snprintf(cd_text, sizeof(cd_text), "%d", cd_num);
        float frac = cd_timer - (int)cd_timer;
        int scaled_size = (int)(100 * (1.0f + frac * 0.5f));
        DrawText(cd_text, (TC_WIDTH - MeasureText(cd_text, scaled_size)) / 2, (TC_HEIGHT - scaled_size) / 2, scaled_size, (Color){ 255, 255, 200, (unsigned char)(255 * (1.0f - frac)) });
    }
}

void tectris_unload(void) {
    if (sound_loaded) UnloadSound(pop_sound);
    printf("[TECTRIS] Game unloaded\n");
}

Game TECTRIS_GAME = {
    .name = "teCtris",
    .description = "Encaixe blocos para limpar as linhas",
    .description_en = "Fit blocks to clear lines",
    .game_width = TC_WIDTH,
    .game_height = TC_HEIGHT,
    .init = tectris_init,
    .update = tectris_update,
    .draw = tectris_draw,
    .unload = tectris_unload,
};
