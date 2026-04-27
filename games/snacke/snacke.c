/**
 * @file snacke.c
 * @brief Snake game module — refactored from standalone snaCke main.c.
 *
 * Features:
 *   - Grid-based snacke with connected pixel-art segments
 *   - Apple (+1), Grape (+3, timed spawn), Orange (+10, rare timed spawn)
 *   - Speed increases every 20 points
 *   - Victory at body length >= 100
 *   - Game over on wall/self collision
 *   - Sound effect on eating
 *   - Highscore tracking (volatile in hub)
 *
 * Internal resolution: 600×600 (square)
 * Controls: WASD or Arrow keys
 *
 * Original by PietroTy (2024), refactored for Chub hub.
 */
#include <stdio.h>
#include <stdbool.h>
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

#define SN_WIDTH  600
#define SN_HEIGHT 600
#define SN_SEG_W  40
#define SN_SEG_H  40
#define SN_GRID   50   // Cell size
#define SN_MAX_BODY 144

// ══════════════════════════════════════════════
// TYPES
// ══════════════════════════════════════════════

typedef struct { float x, y; } SnakeSegment;
typedef struct { int x, y; } SnakeApple;
typedef struct { int x, y; bool isVisible; float spawnTime; } SnakeGrape;
typedef struct { int x, y; bool isVisible; float spawnTime; } SnakeOrange;

typedef struct { float x, y, vx, vy, life; Color col; } SNParticle;
#define MAX_SN_PARTICLES 100

// ══════════════════════════════════════════════
// STATE (all static)
// ══════════════════════════════════════════════

static SnakeSegment head;
static SnakeSegment body[SN_MAX_BODY];
static SnakeSegment trail_segments[5];
static int trail_count;
static SnakeApple apple;
static SnakeGrape grape;
static SnakeOrange orange_fruit;  // 'orange' can conflict with raylib color
static int bodyLength;
static int direction;
static int lastDirection;
static int score;
static int highscore;
static bool gameOver;
static bool victory;
static bool moved;
static bool mouthOpen;
static float moveInterval;
static float lastMoveTime;
static float lastGrapeTime;
static float lastOrangeTime;
static int sn_skin;
static bool sn_store;
static int sn_selection;
static float sn_orange_timer = 0;
static SNParticle particles[MAX_SN_PARTICLES];
static int particle_count = 0;

static Sound pop_sound;
static bool sound_loaded;

// ══════════════════════════════════════════════
// HELPERS
// ══════════════════════════════════════════════

static bool sn_pos_on_snacke(int x, int y) {
    if (x == (int)head.x && y == (int)head.y) return true;
    for (int i = 0; i < bodyLength; i++) {
        if (x == (int)body[i].x && y == (int)body[i].y) return true;
    }
    return false;
}

static void sn_gen_fruit_pos(int *fx, int *fy) {
    do {
        *fx = GetRandomValue(1, 11) * SN_GRID;
        *fy = GetRandomValue(1, 11) * SN_GRID;
    } while (sn_pos_on_snacke(*fx, *fy));
}

static void sn_reset_game(void) {
    head.x = floorf((SN_WIDTH / 2) / SN_GRID) * SN_GRID;
    head.y = floorf((SN_HEIGHT / 2) / SN_GRID) * SN_GRID;
    bodyLength = 2;

    for (int i = 0; i < bodyLength; i++) {
        body[i].x = head.x - ((i + 1) * SN_GRID);
        body[i].y = head.y;
    }

    trail_count = 0;
    int ax, ay;
    sn_gen_fruit_pos(&ax, &ay);
    apple.x = ax;
    apple.y = ay;

    grape.isVisible = false;
    orange_fruit.isVisible = false;
    lastGrapeTime = (float)GetTime();
    lastOrangeTime = (float)GetTime();

    score = 0;
    gameOver = false;
    victory = false;
    moved = false;
    mouthOpen = false;
    moveInterval = 0.2f;
    direction = 1;
    lastDirection = 1;
    lastMoveTime = (float)GetTime();
}

static void sn_draw_segment(SnakeSegment seg, int idx, bool isHead,
                             bool left, bool right, bool up, bool down) {
    Color col_main, col_alt, head_col;
    bool special_render = false;

    switch (sn_skin) {
        case 1: col_main = DARKBLUE; col_alt = BLUE; head_col = SKYBLUE; break;
        case 2: col_main = DARKRED; col_alt = RED; head_col = (Color){255,100,100,255}; break;
        case 3: col_main = DARKPURPLE; col_alt = PURPLE; head_col = (Color){200,100,255,255}; break;
        case 4: col_main = (Color){20,20,20,255}; col_alt = BLACK; head_col = BLACK; break;
        case 5: // Earthworm
            col_main = (Color){210, 140, 140, 255}; 
            col_alt = (Color){230, 160, 160, 255};
            head_col = (Color){240, 180, 180, 255};
            if (idx == 1 || idx == 2) { col_main = col_alt = (Color){180, 100, 100, 255}; } // Clitellum
            break;
        case 6: // Fire
            col_main = RED; col_alt = ORANGE; head_col = YELLOW;
            break;
        case 7: // Skinny Green (formerly Tiger)
            col_main = DARKGREEN; col_alt = LIME; head_col = GREEN;
            break;
        case 8: // Gold
            col_main = GOLD; col_alt = YELLOW; head_col = (Color){255,230,100,255}; break;
        default: col_main = DARKGREEN; col_alt = LIME; head_col = GREEN; break;
    }
    
    Color col = (idx % 2 == 0) ? col_main : col_alt;

    if (isHead) {
        int sw = SN_SEG_W; int sh = SN_SEG_H; int off = 5;
        if (sn_skin == 7) { sw = 20; sh = 20; off = 15; }

        DrawRectangle((int)seg.x + off, (int)seg.y + off, sw, sh, head_col);
        // Connections draw slightly outside/into neighboring cells to ensure no gaps
        if (left)  DrawRectangle((int)seg.x - 2, (int)seg.y + off, off + 2, sh, head_col);
        if (right) DrawRectangle((int)seg.x + off + sw, (int)seg.y + off, off + 2, sh, head_col);
        if (up)    DrawRectangle((int)seg.x + off, (int)seg.y - 2, sw, off + 2, head_col);
        if (down)  DrawRectangle((int)seg.x + off, (int)seg.y + off + sh, sw, off + 2, head_col);
    } else {
        int sw = SN_SEG_W; int sh = SN_SEG_H; int off = 5;
        if (sn_skin == 7) { sw = 20; sh = 20; off = 15; }

        DrawRectangle((int)seg.x + off, (int)seg.y + off, sw, sh, col);
        if (left)  DrawRectangle((int)seg.x - 2, (int)seg.y + off, off + 2, sh, col);
        if (right) DrawRectangle((int)seg.x + off + sw, (int)seg.y + off, off + 2, sh, col);
        if (up)    DrawRectangle((int)seg.x + off, (int)seg.y - 2, sw, off + 2, col);
        if (down)  DrawRectangle((int)seg.x + off, (int)seg.y + off + sh, sw, off + 2, col);
    }
}

// ══════════════════════════════════════════════
// LIFECYCLE
// ══════════════════════════════════════════════

void snacke_init(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    highscore = hub_load_score(1);

    // Load sound
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;

    sn_reset_game();
    sn_skin = 0;
    sn_selection = 0;
    sn_store = false;
    sn_orange_timer = 0;
    printf("[SNACKE] Game initialized\n");
}

void snacke_update(void) {
    if (cd_num > 0) {
        cd_timer += GetFrameTime();
        cd_num = 3 - (int)cd_timer;
        if (cd_num > 0) return;
    }
    float currentTime = (float)GetTime();
    float deltaTime = currentTime - lastMoveTime;

    // Speed increase every 20 points
    if (score > 0 && score % 20 == 0) {
        moveInterval = 0.2f - (0.01f * (score / 20));
        if (moveInterval < 0.05f) moveInterval = 0.05f;
    }

    if (gameOver || victory) {
        if (sn_store) {
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_ENTER)) sn_store = false;
            
            // Arrow navigation
            if (IsKeyPressed(KEY_RIGHT)) sn_selection = (sn_selection + 1) % 9;
            if (IsKeyPressed(KEY_LEFT)) sn_selection = (sn_selection + 8) % 9;
            if (IsKeyPressed(KEY_DOWN)) sn_selection = (sn_selection + 3) % 9;
            if (IsKeyPressed(KEY_UP)) sn_selection = (sn_selection + 6) % 9;

            // Direct selection
            if (IsKeyPressed(KEY_ONE)) sn_selection = 0;
            if (IsKeyPressed(KEY_TWO)) sn_selection = 1;
            if (IsKeyPressed(KEY_THREE)) sn_selection = 2;
            if (IsKeyPressed(KEY_FOUR)) sn_selection = 3;
            if (IsKeyPressed(KEY_FIVE)) sn_selection = 4;
            if (IsKeyPressed(KEY_SIX)) sn_selection = 5;
            if (IsKeyPressed(KEY_SEVEN)) sn_selection = 6;
            if (IsKeyPressed(KEY_EIGHT)) sn_selection = 7;
            if (IsKeyPressed(KEY_NINE)) sn_selection = 8;

            // Thresholds
            int thresholds[] = {0, 15, 30, 45, 60, 80, 100, 120, 141};
            if (highscore >= thresholds[sn_selection]) {
                sn_skin = sn_selection;
            }
        } else {
            if (score > highscore) { highscore = score; hub_save_score(1, highscore); }
            if (IsKeyPressed(KEY_ENTER)) { sn_reset_game(); cd_num = 3; cd_timer = 0.0f; }
            if (IsKeyPressed(KEY_S)) sn_store = true;
        }
        return;
    }

    // Input
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
        if (lastDirection != 2) { direction = 1; moved = true; }
    } else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
        if (lastDirection != 1) { direction = 2; moved = true; }
    } else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        if (lastDirection != 4) { direction = 3; moved = true; }
    } else if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        if (lastDirection != 3) { direction = 4; moved = true; }
    }

    // Movement tick
    if (deltaTime >= moveInterval) {
        mouthOpen = !mouthOpen;
        lastMoveTime = currentTime;
        lastDirection = direction;

        // Shift trail
        for (int i = 4; i > 0; i--) trail_segments[i] = trail_segments[i - 1];
        trail_segments[0] = body[bodyLength - 1];
        if (trail_count < 5) trail_count++;

        // Shift body
        for (int i = bodyLength - 1; i > 0; i--) body[i] = body[i - 1];
        if (bodyLength > 0) { body[0].x = head.x; body[0].y = head.y; }

        if (direction == 1) head.x += SN_GRID;
        else if (direction == 2) head.x -= SN_GRID;
        else if (direction == 3) head.y += SN_GRID;
        else if (direction == 4) head.y -= SN_GRID;

    }

    // (Particles removed)


    // Self-collision
    if (moved && bodyLength > 2) {
        for (int i = 0; i < bodyLength; i++) {
            if (head.x == body[i].x && head.y == body[i].y) gameOver = true;
        }
    }

    // Grape logic (timed spawn)
    if (grape.isVisible && currentTime - grape.spawnTime >= 3.0f) {
        grape.isVisible = false;
        lastGrapeTime = currentTime;
    } else if (!grape.isVisible && currentTime - lastGrapeTime >= 12.0f) {
        int gx, gy;
        sn_gen_fruit_pos(&gx, &gy);
        grape.x = gx; grape.y = gy;
        grape.isVisible = true;
        grape.spawnTime = currentTime;
    }

    // Orange logic (rarer timed spawn)
    if (orange_fruit.isVisible && currentTime - orange_fruit.spawnTime >= 15.0f) {
        orange_fruit.isVisible = false;
        lastOrangeTime = currentTime;
    } else if (!orange_fruit.isVisible && currentTime - lastOrangeTime >= 30.0f) {
        int ox, oy;
        sn_gen_fruit_pos(&ox, &oy);
        orange_fruit.x = ox; orange_fruit.y = oy;
        orange_fruit.isVisible = true;
        orange_fruit.spawnTime = currentTime;
    }

    // Eat apple
    if ((int)head.x == apple.x && (int)head.y == apple.y) {
        score++; if (score > highscore) { highscore = score; hub_save_score(1, highscore); }
        if (sound_loaded) PlaySound(pop_sound);
        body[bodyLength] = body[bodyLength - 1];
        bodyLength++;
        int ax, ay;
        sn_gen_fruit_pos(&ax, &ay);
        apple.x = ax; apple.y = ay;
    }

    // Eat grape
    if (grape.isVisible && (int)head.x == grape.x && (int)head.y == grape.y) {
        score += 3; if (score > highscore) { highscore = score; hub_save_score(1, highscore); } if (score > highscore) { highscore = score; hub_save_score(1, highscore); }
        if (sound_loaded) PlaySound(pop_sound);
        for (int i = 0; i < 3 && bodyLength < SN_MAX_BODY; i++)
            { body[bodyLength] = body[bodyLength - 1]; bodyLength++; }
        grape.isVisible = false;
        lastGrapeTime = currentTime;
    }

    // Eat orange
    if (orange_fruit.isVisible && (int)head.x == orange_fruit.x && (int)head.y == orange_fruit.y) {
        score += 10; if (score > highscore) { highscore = score; hub_save_score(1, highscore); } if (score > highscore) { highscore = score; hub_save_score(1, highscore); }
        if (sound_loaded) PlaySound(pop_sound);
        for (int i = 0; i < 10 && bodyLength < SN_MAX_BODY; i++)
            { body[bodyLength] = body[bodyLength - 1]; bodyLength++; }
        orange_fruit.isVisible = false;
        lastOrangeTime = currentTime;
    }

    // Wall collision
    if (head.x < 0 || head.x >= SN_WIDTH || head.y < 0 || head.y >= SN_HEIGHT) gameOver = true;

    // Victory
    if (bodyLength >= 143) victory = true;
}

void snacke_draw(void) {
    // ── Store screen ──
    if ((gameOver || victory) && sn_store) {
        DrawRectangle(0, 0, SN_WIDTH, SN_HEIGHT, BLACK);
        DrawText(L("LOJA", "SHOP"), (SN_WIDTH - MeasureText(L("LOJA", "SHOP"), 60)) / 2, 40, 60, WHITE);
        DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), (SN_WIDTH - MeasureText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), 30)) / 2, 110, 30, GRAY);

        int thresholds[] = {0, 15, 30, 45, 60, 80, 100, 120, 141};
        Color mainCols[] = {DARKGREEN, DARKBLUE, DARKRED, DARKPURPLE, (Color){45,45,45,255}, (Color){210, 140, 140, 255}, ORANGE, DARKGREEN, GOLD};

        for (int i = 0; i < 9; i++) {
            int row = i / 3;
            int col = i % 3;
            int x = 100 + col * 180;
            int y = 180 + row * 110;

            // Highlight square in background for selection
            if (sn_selection == i) {
                float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
                DrawRectangle(x - 10, y - 10, 60, 60, (Color){200, 200, 200, (unsigned char)(60 + 40 * pulse)}); 
            }

            if (highscore >= thresholds[i]) {
                int sw = 40, sh = 40, off_x = 0, off_y = 0;
                if (i == 7) { sw = 20; sh = 20; off_x = 10; off_y = 10; }
                
                DrawRectangle(x + off_x, y + off_y, sw, sh, mainCols[i]);
                
                // Icon mouth (closed)
                Color iconMouthCol = (i == 4) ? GRAY : BLACK;
                int imw = (int)(sw * 0.7f);
                int imh = (int)(sh * 0.2f);
                int imx = x + off_x + (sw - imw) / 2;
                int imy = y + off_y + (int)(sh * 0.55f);
                DrawRectangle(imx, imy, imw, imh, iconMouthCol);

                DrawText(TextFormat("%d", i + 1), x - 30, y + 10, 20, WHITE);
            } else {
                DrawRectangle(x, y, 40, 40, (Color){30, 30, 30, 255});
                DrawText(L("Bloq.", "Locked"), x, y + 45, 18, GRAY);
                DrawText(TextFormat(L("%d pts", "%d pts"), thresholds[i]), x, y + 65, 12, RED);
                if (i == 8) DrawText(L("(Venca o Jogo)", "(Win the Game)"), x, y + 80, 10, GOLD);
                if (sn_selection == i) DrawRectangleLinesEx((Rectangle){(float)x - 8, (float)y - 8, 56, 56}, 2, GRAY);
            }
        }

        DrawText(L("SETAS para navegar | ENTER para selecionar e sair", "ARROWS to navigate | ENTER to select and exit"), (SN_WIDTH - MeasureText(L("SETAS para navegar | ENTER para selecionar e sair", "ARROWS to navigate | ENTER to select and exit"), 20)) / 2, SN_HEIGHT - 45, 20, GRAY);
        return;
    }

    // ── Game over ──
    if (gameOver || victory) {
        DrawRectangle(0, 0, SN_WIDTH, SN_HEIGHT, (Color){0, 0, 0, 180});
        
        int center_y = SN_HEIGHT / 2 - 80;
        const char *title = victory ? L("VITORIA!", "VICTORY!") : L("FIM DE JOGO", "GAME OVER");
        Color title_col = victory ? GOLD : RED;
        int tw = MeasureText(title, 60);
        DrawText(title, (SN_WIDTH - tw) / 2, center_y, 60, title_col);
        
        DrawText(TextFormat(L("Pontos: %d", "Score: %d"), score), (SN_WIDTH - MeasureText(TextFormat(L("Pontos: %d", "Score: %d"), score), 40)) / 2, center_y + 80, 40, WHITE);
        DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), (SN_WIDTH - MeasureText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), 24)) / 2, center_y + 130, 24, GRAY);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        Color pulse_c = (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)};
        DrawText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), (SN_WIDTH - MeasureText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), 20)) / 2, center_y + 190, 20, pulse_c);
        DrawText(L("Pressione S para Loja", "Press S for Shop"), (SN_WIDTH - MeasureText(L("Pressione S para Loja", "Press S for Shop"), 20)) / 2, center_y + 220, 20, pulse_c);
        
        return;
    }

    // ── Active game ──
    ClearBackground(DARKBROWN);

    // Grid
    Color gridCol = (Color){ 66, 53, 37, 255 };
    for (int i = 0; i <= SN_WIDTH; i += 100) {
        for (int j = 0; j <= SN_HEIGHT; j += 100) DrawRectangle(i, j, 50, 50, gridCol);
        for (int j = 0; j <= SN_HEIGHT; j += 100) DrawRectangle(i + 50, j + 50, 50, 50, gridCol);
    }

    // Fire particles
    for (int i = 0; i < particle_count; i++) {
        DrawCircle((int)particles[i].x, (int)particles[i].y, 3.0f * particles[i].life, particles[i].col);
    }

    // Trail / Path Highlight
    Color trailCol = (Color){ 50, 40, 30, 255 };
    // Under head
    DrawRectangle((int)head.x, (int)head.y, SN_GRID, SN_GRID, trailCol);
    // Under body
    for (int i = 0; i < bodyLength; i++) {
        DrawRectangle((int)body[i].x, (int)body[i].y, SN_GRID, SN_GRID, trailCol);
    }
    // Under vacated tail positions
    for (int i = 0; i < trail_count; i++) {
        DrawRectangle((int)trail_segments[i].x, (int)trail_segments[i].y, SN_GRID, SN_GRID, trailCol);
    }

    // Head
    bool hl = (bodyLength > 0 && head.x > body[0].x);
    bool hr = (bodyLength > 0 && head.x < body[0].x);
    bool hu = (bodyLength > 0 && head.y > body[0].y);
    bool hd = (bodyLength > 0 && head.y < body[0].y);
    sn_draw_segment(head, 0, true, hl, hr, hu, hd);

    // Head mouth
    Color mouthCol = (sn_skin == 4) ? GRAY : BLACK;
    int mw = 20, mh = 20, moff_x = 15, moff_y = 15;
    if (sn_skin == 7) { mw = 10; mh = 10; moff_x = 20; moff_y = 20; }
    
    if (mouthOpen) DrawRectangle((int)head.x + moff_x, (int)head.y + moff_y, mw, mh, mouthCol);
    else {
        int cw = 30, ch = 10, coff_x = 10, coff_y = 20;
        if (sn_skin == 7) { cw = 14; ch = 6; coff_x = 18; coff_y = 22; }
        DrawRectangle((int)head.x + coff_x, (int)head.y + coff_y, cw, ch, mouthCol);
    }

    // Body
    for (int i = 0; i < bodyLength; i++) {
        SnakeSegment prev = (i == 0) ? head : body[i - 1];
        bool hasNext = (i < bodyLength - 1);
        SnakeSegment next = hasNext ? body[i + 1] : body[i];

        bool l = (body[i].x > prev.x) || (hasNext && body[i].x > next.x);
        bool r = (body[i].x < prev.x) || (hasNext && body[i].x < next.x);
        bool u = (body[i].y > prev.y) || (hasNext && body[i].y > next.y);
        bool d = (body[i].y < prev.y) || (hasNext && body[i].y < next.y);
        
        sn_draw_segment(body[i], i + 1, false, l, r, u, d);
    }

    // Apple
    DrawRectangle(apple.x + 5, apple.y + 5, 40, 40, RED);
    DrawRectangle(apple.x + 20, apple.y, 20, 10, LIME);

    // Grape
    if (grape.isVisible) {
        DrawRectangle(grape.x + 5, grape.y + 5, 18, 18, PURPLE);
        DrawRectangle(grape.x + 27, grape.y + 5, 18, 18, PURPLE);
        DrawRectangle(grape.x + 16, grape.y + 28, 18, 18, PURPLE);
        DrawRectangle(grape.x + 20, grape.y, 20, 10, LIME);
    }

    // Orange
    if (orange_fruit.isVisible) {
        DrawRectangle(orange_fruit.x + 5, orange_fruit.y + 5, 40, 40, ORANGE);
        DrawRectangle(orange_fruit.x + 20, orange_fruit.y, 20, 10, LIME);
    }

    // HUD
    DrawText(TextFormat(L("Pontos: %d", "Score: %d"), score), 10, 10, 30, WHITE);
    const char *hs_text = TextFormat(L("Recorde: %d", "Highscore: %d"), highscore);
    DrawText(hs_text, SN_WIDTH - MeasureText(hs_text, 30) - 10, 10, 30, WHITE);




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
        DrawText(cd_text, (SN_WIDTH - scaled_w) / 2, (SN_HEIGHT - scaled_size) / 2, scaled_size, cd_col);
    }
}

void snacke_unload(void) {
    if (sound_loaded) {
        UnloadSound(pop_sound);
        sound_loaded = false;
    }
    printf("[SNACKE] Game unloaded\n");
}

// ══════════════════════════════════════════════
// GAME MODULE DEFINITION
// ══════════════════════════════════════════════

Game SNACKE_GAME = {
    .name           = "snaCke",
    .description    = "Coma frutas e cresca sem bater no corpo",
    .description_en = "Eat fruits and grow without hitting yourself",
    .game_width     = SN_WIDTH,
    .game_height    = SN_HEIGHT,
    .init           = snacke_init,
    .update         = snacke_update,
    .draw           = snacke_draw,
    .unload         = snacke_unload,
};
