/**
 * @file ctron.c
 * @brief Tron light-cycle game — 2 players on a grid.
 * P1: WASD, P2: Arrows. Leave a trail, don't crash!
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib/raylib.h"
#include "game.h"
#include "hub.h"
static int cd_num;
static float cd_timer;

#define TW 640
#define TH 480
#define CELL 5
#define ROWS (TH/CELL)
#define COLS (TW/CELL)

#define COL_P1 (Color){80,200,255,255}
#define COL_P2 (Color){100,255,100,255}

typedef enum { TR_START, TR_RUNNING, TR_P1_WIN, TR_P2_WIN, TR_DRAW } TronState;

static int grid[ROWS*COLS];
static float p1x, p1y, p2x, p2y;
static int p1dir, p2dir; /* 0=up 1=right 2=down 3=left */
static float move_timer;
static float move_interval;
static TronState tstate;
static int game_mode;
static int in_menu;
static Sound pop_sound;
static bool sound_loaded = false;
static int p1_score, p2_score;

static void ctron_reset(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    for (int i = 0; i < ROWS*COLS; i++) grid[i] = 0;
    p1x = TW/4; p1y = TH/2;
    p2x = 3*TW/4; p2y = TH/2;
    p1dir = 1; p2dir = 3;
    move_timer = 0;
    move_interval = 0.02f;
    tstate = TR_RUNNING;
}

void ctron_init(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    p1_score = 0; p2_score = 0;
    ctron_reset();
    in_menu = 1;
    cd_num = 0;
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;
    printf("[CTRON] Game initialized\n");
}

void ctron_update(void) {
    if (cd_num > 0) {
        cd_timer += GetFrameTime();
        cd_num = 3 - (int)cd_timer;
        if (cd_num > 0) return;
    }
    float dt = GetFrameTime();


        if (in_menu) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            game_mode = 0;
            in_menu = 0;
            cd_num = 3;
            cd_timer = 0;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            game_mode = 1;
            in_menu = 0;
            cd_num = 3;
            cd_timer = 0;
        }
        return;
    }

    if (tstate != TR_RUNNING) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) { ctron_reset(); cd_num = 3; cd_timer = 0.0f; }
        return;
    }

    /* P1 input */
    if (IsKeyPressed(KEY_W) && p1dir != 2) { p1dir = 0; if (sound_loaded) PlaySound(pop_sound); }
    if (IsKeyPressed(KEY_D) && p1dir != 3) { p1dir = 1; if (sound_loaded) PlaySound(pop_sound); }
    if (IsKeyPressed(KEY_S) && p1dir != 0) { p1dir = 2; if (sound_loaded) PlaySound(pop_sound); }
    if (IsKeyPressed(KEY_A) && p1dir != 1) { p1dir = 3; if (sound_loaded) PlaySound(pop_sound); }
    
    if (game_mode == 0) {
        if (IsKeyPressed(KEY_UP)    && p1dir != 2) p1dir = 0;
        if (IsKeyPressed(KEY_RIGHT) && p1dir != 3) p1dir = 1;
        if (IsKeyPressed(KEY_DOWN)  && p1dir != 0) p1dir = 2;
        if (IsKeyPressed(KEY_LEFT)  && p1dir != 1) p1dir = 3;
    }

    /* P2 input */
    if (game_mode == 1) {
        if (IsKeyPressed(KEY_UP)    && p2dir != 2) { p2dir = 0; if (sound_loaded) PlaySound(pop_sound); }
        if (IsKeyPressed(KEY_RIGHT) && p2dir != 3) { p2dir = 1; if (sound_loaded) PlaySound(pop_sound); }
        if (IsKeyPressed(KEY_DOWN)  && p2dir != 0) { p2dir = 2; if (sound_loaded) PlaySound(pop_sound); }
        if (IsKeyPressed(KEY_LEFT)  && p2dir != 1) { p2dir = 3; if (sound_loaded) PlaySound(pop_sound); }
    } else {
        /* Smarter Tron AI: Look ahead and evaluate free space */
        float dx_arr[] = {0, CELL, 0, -CELL};
        float dy_arr[] = {-CELL, 0, CELL, 0};

        /* 1. Check for immediate danger in current direction */
        int danger = 0;
        int check_dist = 6; /* look ahead further */
        for (int i = 1; i <= check_dist; i++) {
            int nx = (int)(p2x + dx_arr[p2dir] * i);
            int ny = (int)(p2y + dy_arr[p2dir] * i);
            int nr = ny / CELL, nc = nx / CELL;
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS || grid[nr * COLS + nc] != 0) {
                danger = 1;
                break;
            }
        }

        /* 2. Occasional random turn for unpredictability (1% chance) */
        if (!danger && GetRandomValue(0, 100) < 1) danger = 1;

        if (danger) {
            /* Evaluate Left and Right turns */
            int dir_left = (p2dir + 3) % 4;
            int dir_right = (p2dir + 1) % 4;
            
            int score_left = 0, score_right = 0;
            
            /* Enhanced Scan: Check a wider area in the turn directions to find 'open' space */
            for (int i = 1; i <= 15; i++) {
                for (int j = -2; j <= 2; j++) {
                    // Left direction scan
                    int lx = (int)(p2x + dx_arr[dir_left] * i + dx_arr[p2dir] * j);
                    int ly = (int)(p2y + dy_arr[dir_left] * i + dy_arr[p2dir] * j);
                    int lr = ly / CELL, lc = lx / CELL;
                    if (lr >= 0 && lr < ROWS && lc >= 0 && lc < COLS && grid[lr * COLS + lc] == 0) score_left++;

                    // Right direction scan
                    int rx = (int)(p2x + dx_arr[dir_right] * i + dx_arr[p2dir] * j);
                    int ry = (int)(p2y + dy_arr[dir_right] * i + dy_arr[p2dir] * j);
                    int rr = ry / CELL, rc = rx / CELL;
                    if (rr >= 0 && rr < ROWS && rc >= 0 && rc < COLS && grid[rr * COLS + rc] == 0) score_right++;
                }
            }

            if (score_left > score_right) p2dir = dir_left;
            else if (score_right > score_left) p2dir = dir_right;
            else p2dir = (GetRandomValue(0, 1) == 0) ? dir_left : dir_right;
        }
    }

    move_timer += dt;
    if (move_timer < move_interval) return;
    move_timer -= move_interval;

    /* move */
    float dx[] = {0, CELL, 0, -CELL};
    float dy[] = {-CELL, 0, CELL, 0};
    p1x += dx[p1dir]; p1y += dy[p1dir];
    p2x += dx[p2dir]; p2y += dy[p2dir];

    /* check collisions */
    int p1r = (int)(p1y/CELL), p1c = (int)(p1x/CELL);
    int p2r = (int)(p2y/CELL), p2c = (int)(p2x/CELL);

    int p1_dead = 0, p2_dead = 0;

    if (p1r < 0 || p1r >= ROWS || p1c < 0 || p1c >= COLS || grid[p1r*COLS+p1c] != 0) p1_dead = 1;
    if (p2r < 0 || p2r >= ROWS || p2c < 0 || p2c >= COLS || grid[p2r*COLS+p2c] != 0) p2_dead = 1;

    if (!p1_dead) grid[p1r*COLS+p1c] = 1;
    if (!p2_dead) grid[p2r*COLS+p2c] = 2;

    if (p1_dead && p2_dead) { tstate = TR_DRAW; }
    else if (p1_dead) { tstate = TR_P2_WIN; p2_score++; }
    else if (p2_dead) { tstate = TR_P1_WIN; p1_score++; }
}

void ctron_draw(void) {
    ClearBackground(BLACK);

    if (in_menu) {
        DrawText(L("SELECIONE O MODO", "SELECT MODE"), TW / 2 - MeasureText(L("SELECIONE O MODO", "SELECT MODE"), 40) / 2, 100, 40, WHITE);
        Color c1 = (game_mode == 0) ? PURPLE : WHITE;
        Color c2 = (game_mode == 1) ? PURPLE : WHITE;
        DrawText(L("<- 1 Jogador (Vs CPU)", "<- 1 Player (Vs CPU)"), TW / 2 - 250, 250, 20, c1);
        DrawText(L("2 Jogadores ->", "2 Players ->"), TW / 2 + 50, 250, 20, c2);
        DrawText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), TW / 2 - MeasureText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), 20) / 2, 400, 20, DARKGRAY);
        return;
    }


    /* grid trail */
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int v = grid[r*COLS+c];
            if (v == 1) DrawRectangle(c*CELL, r*CELL, CELL, CELL, COL_P1);
            else if (v == 2) DrawRectangle(c*CELL, r*CELL, CELL, CELL, COL_P2);
        }
    }

    /* player heads */
    if (tstate == TR_RUNNING) {
        DrawRectangle((int)p1x, (int)p1y, CELL, CELL, WHITE);
        DrawRectangle((int)p2x, (int)p2y, CELL, CELL, WHITE);
    }

    /* scores */
    DrawText(TextFormat(L("P1: %d", "P1: %d"), p1_score), TW/4 - 40, 10, 40, COL_P1);
    const char *p2_label = (game_mode == 1) ? L("P2", "P2") : L("CPU", "CPU");
    DrawText(TextFormat(L("%s: %d", "%s: %d"), p2_label, p2_score), 3*TW/4-100, 10, 40, COL_P2);



    if (tstate == TR_P1_WIN || tstate == TR_P2_WIN || tstate == TR_DRAW) {
        DrawRectangle(0, 0, TW, TH, (Color){0, 0, 0, 200});
        const char *p2_win_name = (game_mode == 1) ? L("P2", "P2") : L("CPU", "CPU");
        const char *msg = tstate == TR_P1_WIN ? L("P1 VENCEU!", "P1 WINS!") : (tstate == TR_P2_WIN ? TextFormat(L("%s VENCEU!", "%s WINS!"), p2_win_name) : L("EMPATE!", "DRAW!"));
        Color mc = tstate == TR_P1_WIN ? COL_P1 : tstate == TR_P2_WIN ? COL_P2 : ORANGE;
        DrawText(msg, (TW-MeasureText(msg,40))/2, TH/2-30, 40, mc);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        const char *hint = L("Pressione ENTER para Reiniciar", "Press ENTER to Restart");
        int hw = MeasureText(hint, 20);
        DrawText(hint, (TW - hw)/2, TH/2+30, 20, (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)});
    }



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
        DrawText(cd_text, (TW - scaled_w) / 2, (TH - scaled_size) / 2, scaled_size, cd_col);
    }
}

void ctron_unload(void) { 
    if (sound_loaded) UnloadSound(pop_sound);
    printf("[CTRON] Game unloaded\n"); 
}

Game CTRON_GAME = {
    .name = "Ctron",
    .description = "Deixe um rastro de luz e vença seu rival",
    .description_en = "Leave a light trail and beat your rival",
    .game_width = TW, .game_height = TH,
    .init = ctron_init, .update = ctron_update,
    .draw = ctron_draw, .unload = ctron_unload,
};
