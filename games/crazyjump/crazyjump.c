/**
 * @file crazyjump.c
 * @brief DoodleJump clone — infinite vertical platformer with rectangles.
 * AD or Arrows to move. Jump on platforms to go higher!
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib/raylib.h"
#include "chub_colors.h"
#include "game.h"
#include "hub.h"

static int cd_num;
static float cd_timer;

#define DW 400
#define DH 600
#define PLAT_COUNT 20
#define PLAT_W 64
#define PLAT_H 10
#define PLAYER_W 16
#define PLAYER_H 16
#define GRAVITY 800.0f
#define JUMP_VEL -560.0f
#define MOVE_SPEED 320.0f

typedef struct { 
    float x, y; 
    int type; /* 0=Normais 1=Movem 2=Pedras 3=Nuvens 4=Cogumelos 5=Pequenas */ 
    int broken; 
    float mx; 
} Platform;

static float pPlayerX, pPlayerY, pVelY;
static Platform platforms[PLAT_COUNT];
static float camera_y;
static int dj_score, dj_highscore;
static Sound pop_sound;
static bool sound_loaded = false;
static int dj_dead;
static int dj_skin = 0;
static int dj_selection = 0;
static bool dj_store = false;

static float gameTime;

typedef struct {
    float x;
    float y;
} DJCloud;
static DJCloud dj_clouds[12];

static Color dj_lerp_color(Color c1, Color c2, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    Color r;
    r.r = (unsigned char)(c1.r + (c2.r - c1.r) * t);
    r.g = (unsigned char)(c1.g + (c2.g - c1.g) * t);
    r.b = (unsigned char)(c1.b + (c2.b - c1.b) * t);
    r.a = (unsigned char)(c1.a + (c2.a - c1.a) * t);
    return r;
}
static int dj_facing_right;
static int unsafe_row = 0;

static void gen_platform(Platform *p, float y_base) {
    p->x = (float)GetRandomValue(10, DW - PLAT_W - 10);
    p->y = y_base;
    p->broken = 0;
    p->mx = (float)GetRandomValue(0, 100) / 100.0f * 3.14159f * 2.0f;

    int r = GetRandomValue(0, 100);
    
    // Progressive difficulty: Normal platforms become rarer, Pedras and Pequenas more common
    float diff = dj_score / 2000.0f;
    if (diff > 0.8f) diff = 0.8f; 

    int w_pedra = (int)(25 + 25 * diff);   // 25 -> 45
    int w_nuvem = (int)(10 + 10 * diff);   // 10 -> 18
    int w_pequena = (int)(10 + 20 * diff); // 10 -> 26
    int w_cogumelo = 5;
    int w_movem = 20;

    if (unsafe_row >= 1) {
        if (GetRandomValue(0, 100) < 20) p->type = 0; // Normais (Dramatically reduced)
        else p->type = 1; // Movem
        unsafe_row = 0;
    } else {
        if (r < w_pedra) { p->type = 2; unsafe_row++; }      /* Pedras */
        else if (r < w_pedra + w_nuvem) { p->type = 3; unsafe_row++; } /* Nuvens */
        else if (r < w_pedra + w_nuvem + w_pequena) { p->type = 5; unsafe_row++; } /* Pequenas */
        else if (r < w_pedra + w_nuvem + w_pequena + w_cogumelo) { p->type = 4; unsafe_row = 0; } /* Cogumelos */
        else if (r < w_pedra + w_nuvem + w_pequena + w_cogumelo + w_movem) { p->type = 1; unsafe_row = 0; } /* Movem */
        else { p->type = 0; unsafe_row = 0; }            /* Normais */
    }
}

static void dj_reset(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    pPlayerX = DW/2 - PLAYER_W/2;
    pPlayerY = DH - 100;
    pVelY = JUMP_VEL;
    camera_y = 0;
    dj_score = 0;
    dj_dead = 0;
    
    gameTime = 0.0f;
    for (int i = 0; i < 12; i++) {
        dj_clouds[i].x = (float)GetRandomValue(0, DW);
        dj_clouds[i].y = (float)GetRandomValue(0, DH);
    }
    dj_facing_right = 1;
    unsafe_row = 0;

    /* generate platforms */
    for (int i = 1; i < PLAT_COUNT; i++) {
        gen_platform(&platforms[i], DH - 100 - (float)i * 70.0f);
        if (i < 5) platforms[i].type = 0; 
    }
    platforms[0].x = pPlayerX - 10;
    platforms[0].y = pPlayerY + PLAYER_H + 5;
    platforms[0].type = 0;
}

void crazyjump_init(void) {
    cd_num = 3;
    cd_timer = 0.0f;
    dj_highscore = hub_load_score(4);
    dj_reset();
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;
    printf("[CRAZYJUMP] Game initialized (Score forced to 20000)\n");
}

void crazyjump_update(void) {
    if (cd_num > 0) {
        cd_timer += GetFrameTime();
        cd_num = 3 - (int)cd_timer;
        if (cd_num > 0) return;
    }
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    gameTime += dt;

    if (dj_dead) {
        if (dj_store) {
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_ENTER)) dj_store = false;
            if (IsKeyPressed(KEY_RIGHT)) dj_selection = (dj_selection + 1) % 9;
            if (IsKeyPressed(KEY_LEFT)) dj_selection = (dj_selection + 8) % 9;
            if (IsKeyPressed(KEY_DOWN)) dj_selection = (dj_selection + 3) % 9;
            if (IsKeyPressed(KEY_UP)) dj_selection = (dj_selection + 6) % 9;

            int thresholds[] = {0, 400, 800, 1300, 2000, 2800, 3800, 4800, 6000};
            if (dj_highscore >= thresholds[dj_selection]) dj_skin = dj_selection;
        } else {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) { dj_reset(); cd_num = 3; cd_timer = 0.0f; }
            if (IsKeyPressed(KEY_S)) dj_store = true;
        }
        return;
    }

    /* move */
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) { pPlayerX -= MOVE_SPEED * dt; dj_facing_right = 0; }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { pPlayerX += MOVE_SPEED * dt; dj_facing_right = 1; }

    /* wrap around */
    if (pPlayerX < -PLAYER_W) pPlayerX = DW;
    if (pPlayerX > DW) pPlayerX = -PLAYER_W;

    /* gravity */
    pVelY += GRAVITY * dt;
    pPlayerY += pVelY * dt;

    /* platform collision (only when falling) */
    if (pVelY > 0) {
        for (int i = 0; i < PLAT_COUNT; i++) {
            if (platforms[i].broken) continue;
            
            float pw = (platforms[i].type == 5 || platforms[i].type == 4) ? PLAT_W / 2 : PLAT_W;
            float plat_x = platforms[i].x;
            if (platforms[i].type == 5 || platforms[i].type == 4) plat_x += PLAT_W / 4;
            
            float plat_y = platforms[i].y + camera_y;

            if (pPlayerX + PLAYER_W > plat_x && pPlayerX < plat_x + pw &&
                pPlayerY + PLAYER_H >= plat_y && pPlayerY + PLAYER_H <= plat_y + PLAT_H + pVelY * dt + 5) {
                
                if (platforms[i].type == 2) {
                    platforms[i].broken = 1;
                } else if (platforms[i].type == 3) {
                    pVelY = JUMP_VEL;
                    pPlayerY = plat_y - PLAYER_H;
                    platforms[i].broken = 1; 
                    if (sound_loaded) { SetSoundPitch(pop_sound, 1.4f); PlaySound(pop_sound); }
                } else if (platforms[i].type == 4) {
                    pVelY = JUMP_VEL * 1.8f; 
                    pPlayerY = plat_y - PLAYER_H;
                    if (sound_loaded) { SetSoundPitch(pop_sound, 1.6f); PlaySound(pop_sound); }
                } else {
                    pVelY = JUMP_VEL;
                    pPlayerY = plat_y - PLAYER_H;
                    if (sound_loaded) { SetSoundPitch(pop_sound, 1.0f + (GetRandomValue(-10, 10)/100.0f)); PlaySound(pop_sound); }
                }
            }
        }
    }

    /* scroll camera up */
    if (pPlayerY < DH / 3) {
        float diff = DH / 3 - pPlayerY;
        camera_y += diff;
        pPlayerY = DH / 3;
        dj_score = (int)(camera_y / 10);
        if (dj_score > dj_highscore) { dj_highscore = dj_score; hub_save_score(4, dj_highscore); }
    }

    /* recycle platforms that went below screen */
    for (int i = 0; i < PLAT_COUNT; i++) {
        float screen_y = platforms[i].y + camera_y;
        if (screen_y > DH + 50) {
            float highest = 99999.0f;
            for (int j = 0; j < PLAT_COUNT; j++)
                if (platforms[j].y < highest) highest = platforms[j].y;
            gen_platform(&platforms[i], highest - (float)GetRandomValue(50, 100));
        }
    }

    /* moving and broken platforms */
    for (int i = 0; i < PLAT_COUNT; i++) {
        if (platforms[i].type == 1 && !platforms[i].broken) {
            platforms[i].mx += dt * 2.0f;
            platforms[i].x = (DW - PLAT_W)/2.0f + sinf(platforms[i].mx) * (DW - PLAT_W)/2.5f;
        }
        if (platforms[i].broken && (platforms[i].type == 2 || platforms[i].type == 3)) {
            platforms[i].y += 400.0f * dt;
        }
    }

    /* move clouds horizontally */
    for (int i = 0; i < 12; i++) {
        dj_clouds[i].x -= 20.0f * dt;
        if (dj_clouds[i].x < -100) dj_clouds[i].x = DW;
    }

    /* death */
    if (pPlayerY > DH + 50) dj_dead = 1;
}

static void dj_draw_player(float px, float py, int skin_id, int facing_right, bool dead, float scale, float velY, bool isStore) {
    if (dead) return;
    
    Color body_col, pants_col;
    bool has_snout = false, has_cape = false, has_crown = false, has_horns = false, has_halo = false, has_wings = false;
    bool is_pou = false, is_bee = false, is_bird = false, is_frog = false, is_ball = false;
    Color black_c = isStore ? (Color){45, 45, 45, 255} : BLACK;

    switch(skin_id) {
        case 1: body_col = black_c; pants_col = black_c; break; // Ninja
        case 2: is_frog = true; break; // Frog
        case 3: body_col = WHITE; pants_col = WHITE; has_halo = true; has_wings = true; break; // Angel
        case 4: body_col = RED; pants_col = MAROON; has_horns = true; break; // Devil
        case 5: is_ball = true; break; // Red Ball
        case 6: is_bee = true; break; // Bee
        case 7: is_bird = true; break; // Flappy (Crappy) Bird
        case 8: is_pou = true; break; // Pou
        default: body_col = (Color){255,220,80,255}; pants_col = LIME; has_snout = true; break; // Classic
    }
    
    float pw = PLAYER_W * scale;
    float ph = PLAYER_H * scale;

    if (is_frog) {
        Color frog_c = LIME;
        Color belly_c = (Color){150, 255, 150, 255};
        DrawRectangle((int)px, (int)py, (int)pw, (int)ph, frog_c);
        // Belly
        DrawRectangle((int)(px + pw*0.2f), (int)(py + ph*0.5f), (int)(pw*0.6f), (int)(ph*0.4f), belly_c);
        
        // Frog legs (Animate when jumping: velY < 0, only if not store)
        float leg_ext = (!isStore && velY < -100.0f) ? 4.0f * scale : 0.0f;
        DrawRectangle((int)px - (int)(4*scale), (int)(py + ph - 6*scale + leg_ext), (int)(4*scale), (int)(6*scale), frog_c);
        DrawRectangle((int)px + (int)pw, (int)(py + ph - 6*scale + leg_ext), (int)(4*scale), (int)(6*scale), frog_c);
        /* eyes */
        int ex = facing_right ? (int)px + (int)pw - (int)(6*scale) : (int)px + (int)(2*scale);
        int ey = (int)(py + 2*scale);
        DrawRectangle(ex, ey, (int)(4*scale), (int)(4*scale), WHITE);
        DrawRectangle(ex + (facing_right ? (int)(2*scale) : 0), ey + (int)(1*scale), (int)(2*scale), (int)(2*scale), black_c);
        int ex2 = ex + (facing_right ? -(int)(6*scale) : (int)(6*scale));
        DrawRectangle(ex2, ey, (int)(4*scale), (int)(4*scale), WHITE);
        DrawRectangle(ex2 + (facing_right ? (int)(2*scale) : 0), ey + (int)(1*scale), (int)(2*scale), (int)(2*scale), black_c);
    } else if (is_ball) {
        Color ball_c = RED;
        int sq = (int)(4 * scale);
        // "Circular" arrangement of squares
        DrawRectangle((int)(px + sq), (int)py, (int)(pw - sq*2), sq, ball_c);
        DrawRectangle((int)px, (int)(py + sq), (int)pw, (int)(ph - sq*2), ball_c);
        DrawRectangle((int)(px + sq), (int)(py + ph - sq), (int)(pw - sq*2), sq, ball_c);
        /* eyes - Red Ball */
        int ex = facing_right ? (int)(px + pw - 8*scale) : (int)px + (int)(4*scale);
        int ey = (int)(py + ph/2 - (int)(2*scale));
        DrawRectangle(ex, ey, (int)(4*scale), (int)(4*scale), WHITE);
        DrawRectangle(ex + (facing_right ? (int)(2*scale) : 0), ey + (int)(1*scale), (int)(2*scale), (int)(2*scale), black_c);
    } else if (is_bee) {
        DrawRectangle((int)px, (int)py, (int)pw, (int)ph, YELLOW);
        // Vertical stripes
        DrawRectangle((int)(px + pw*0.2f), (int)py, (int)(pw*0.2f), (int)ph, black_c);
        DrawRectangle((int)(px + pw*0.6f), (int)py, (int)(pw*0.2f), (int)ph, black_c);
        
        // Flappy Wings (Shifted up)
        bool wing_up = !isStore && (fmodf(gameTime, 0.2f) < 0.1f);
        float s = scale * (16.0f / 40.0f);
        int wx = facing_right ? (int)(px - 15*s) : (int)(px + 25*s);
        int wy = (int)(py + (wing_up ? 0*s : 10*s)); // wy shifted up by 10 units in 40-scale
        DrawRectangle(wx, wy, (int)(30*s), (int)(20*s), WHITE); 

        /* Big eye (Flappy style) - On the edge */
        int ex = facing_right ? (int)(px + pw - 18*s) : (int)px;
        int ey = (int)(py + (ph - 18*s)/2);
        DrawRectangle(ex, ey, (int)(18*s), (int)(18*s), WHITE);
        int pux = facing_right ? (int)(ex + 9*s) : (int)ex;
        DrawRectangle(pux, ey + (int)(4*s), (int)(9*s), (int)(9*s), black_c);
    } else if (is_bird) {
        // Authentic Crappy Bird implementation (scaled from 40x40 to 16x16)
        float s = scale * (16.0f / 40.0f); // Conversion factor
        DrawRectangle((int)px, (int)py, (int)(40*s), (int)(40*s), YELLOW);
        
        // Eye
        int ex = (int)(px + 10*s);
        DrawRectangle(ex, (int)(py + 10*s), (int)(20*s), (int)(20*s), WHITE);
        
        // Pupil
        int pux = facing_right ? (int)(px + 20*s) : (int)(px + 10*s);
        DrawRectangle(pux, (int)(py + 10*s), (int)(10*s), (int)(10*s), black_c);
        
        // Beak
        int bx = facing_right ? (int)(px + 30*s) : (int)(px - 10*s);
        DrawRectangle(bx, (int)(py + 20*s), (int)(20*s), (int)(10*s), ORANGE);
        
        // Wing
        bool wing_up = !isStore && (fmodf(gameTime, 0.2f) < 0.1f);
        int wx = facing_right ? (int)(px - 15*s) : (int)(px + 25*s);
        int wy = (int)(py + (wing_up ? 10*s : 20*s));
        DrawRectangle(wx, wy, (int)(30*s), (int)(20*s), ORANGE);
    } else if (is_pou) {
        Color pou_c = (Color){139, 69, 19, 255}; // Brown
        // Draw Pou as a pyramid of squares
        int sq = (int)(4 * scale);
        // Top row
        DrawRectangle((int)(px + pw/2 - sq/2), (int)py, sq, sq, pou_c);
        // Mid row
        DrawRectangle((int)(px + pw/2 - sq*1.5f), (int)(py + sq), sq*3, sq, pou_c);
        // Bottom row
        DrawRectangle((int)(px + pw/2 - sq*2.5f), (int)(py + sq*2), sq*5, sq, pou_c);
        // Base row
        DrawRectangle((int)px, (int)(py + sq*3), (int)pw, sq, pou_c);

        /* eyes for Pou (Two normal eyes) */
        int ex1 = (int)(px + pw/2 - (int)(6 * scale));
        int ex2 = (int)(px + pw/2 + (int)(2 * scale));
        int ey = (int)(py + sq * 1.5f);
        
        // Eye 1
        DrawRectangle(ex1, ey, (int)(4*scale), (int)(4*scale), WHITE);
        DrawRectangle(ex1 + (facing_right ? (int)(2*scale) : 0), ey + (int)(1*scale), (int)(2*scale), (int)(2*scale), black_c);
        // Eye 2
        DrawRectangle(ex2, ey, (int)(4*scale), (int)(4*scale), WHITE);
        DrawRectangle(ex2 + (facing_right ? (int)(2*scale) : 0), ey + (int)(1*scale), (int)(2*scale), (int)(2*scale), black_c);
    } else {
        // Body (Yellow top)
        DrawRectangle((int)px, (int)py, (int)pw, (int)(ph/2 + 2*scale), body_col);
        // Pants (Green bottom)
        DrawRectangle((int)px, (int)(py + ph/2 + 2*scale), (int)pw, (int)(ph/2 - 2*scale), pants_col);
        
        if (has_snout) {
            int snout_x = facing_right ? (int)px + (int)pw : (int)px - (int)(6*scale);
            DrawRectangle(snout_x, (int)(py + 5*scale), (int)(6*scale), (int)(4*scale), body_col);
        }
        if (has_cape) {
            int cx = facing_right ? (int)px - (int)(4*scale) : (int)px + (int)pw;
            DrawRectangle(cx, (int)(py + 4*scale), (int)(4*scale), (int)(12*scale), RED);
        }
        if (has_crown) DrawRectangle((int)px, (int)(py - 4*scale), (int)pw, (int)(4*scale), GOLD);
        if (has_horns) {
            DrawRectangle((int)px, (int)(py - 4*scale), (int)(4*scale), (int)(4*scale), black_c);
            DrawRectangle((int)px + (int)pw - (int)(4*scale), (int)(py - 4*scale), (int)(4*scale), (int)(4*scale), black_c);
        }
        if (has_halo) DrawRectangle((int)px - (int)(2*scale), (int)(py - 6*scale), (int)pw + (int)(4*scale), (int)(3*scale), GOLD);
        if (has_wings) {
            bool wing_up = !isStore && (fmodf(gameTime, 0.3f) < 0.15f);
            Color wing_c = (Color){220, 240, 255, 200};
            int wy = (int)(py + ph/2) + (wing_up ? -(int)(4*scale) : (int)(2*scale));
            // Left wing (Narrower)
            DrawRectangle((int)px - (int)(6*scale), wy, (int)(6*scale), (int)(8*scale), wing_c);
            // Right wing (Narrower)
            DrawRectangle((int)px + (int)pw, wy, (int)(6*scale), (int)(8*scale), wing_c);
        }

        /* eyes */
        int ex = facing_right ? (int)px + (int)pw - (int)(6*scale) : (int)px + (int)(2*scale);
        int ey = (int)(py + 3*scale);
        Color eye_bg = (skin_id == 1) ? RED : WHITE;
        DrawRectangle(ex, ey, (int)(4*scale), (int)(4*scale), eye_bg);
        DrawRectangle(ex + (facing_right ? (int)(2*scale) : 0), ey + (int)(1*scale), (int)(2*scale), (int)(2*scale), black_c);
        int ex2 = ex + (facing_right ? -(int)(6*scale) : (int)(6*scale));
        DrawRectangle(ex2, ey, (int)(4*scale), (int)(4*scale), eye_bg);
        DrawRectangle(ex2 + (facing_right ? (int)(2*scale) : 0), ey + (int)(1*scale), (int)(2*scale), (int)(2*scale), black_c);
    }
}

void crazyjump_draw(void) {
    float cycleTime = 120.0f;
    float t = fmodf(gameTime, cycleTime) / cycleTime;

    Color bgCol, sunCol, cloudCol, platNormalCol;
    if (t < 0.5f) {
        bgCol = dj_lerp_color(SKYBLUE, DEEPBLUE, t * 2);
        sunCol = dj_lerp_color(GOLD, DARKORANGE, t * 2);
        cloudCol = dj_lerp_color(WHITE, DARKGRAY, t * 2);
        platNormalCol = dj_lerp_color(LIME, DARKGREEN, t * 2);
    } else {
        bgCol = dj_lerp_color(DEEPBLUE, SKYBLUE, (t - 0.5f) * 2);
        sunCol = dj_lerp_color(DARKORANGE, GOLD, (t - 0.5f) * 2);
        cloudCol = dj_lerp_color(DARKGRAY, WHITE, (t - 0.5f) * 2);
        platNormalCol = dj_lerp_color(DARKGREEN, LIME, (t - 0.5f) * 2);
    }

    ClearBackground(bgCol);

    /* Sun (independent of parallax, moves slowly across sky during day) */
    if (t < 0.6f) {
        float sun_x = DW - (t / 0.6f) * (DW + 100);
        DrawRectangle((int)sun_x, 150, 60, 60, sunCol);
    }

    /* clouds with parallax */
    for (int i = 0; i < 12; i++) {
        float draw_y = fmodf(dj_clouds[i].y + camera_y * 0.15f, DH + 150) - 50;
        DrawRectangle((int)dj_clouds[i].x, (int)draw_y, 60, 15, cloudCol);
        DrawRectangle((int)dj_clouds[i].x + 15, (int)draw_y - 15, 30, 15, cloudCol);
    }

    /* platforms */
    for (int i = 0; i < PLAT_COUNT; i++) {
        float sy = platforms[i].y + camera_y;
        if (sy < -20 || sy > DH + 20) continue;
        
        float pw = (platforms[i].type == 5 || platforms[i].type == 4) ? PLAT_W / 2 : PLAT_W;
        float px_draw = platforms[i].x;
        if (platforms[i].type == 5 || platforms[i].type == 4) px_draw += PLAT_W / 4;

        if (platforms[i].broken) {
            /* broken fragments */
            Color fragCol = (platforms[i].type == 2) ? LIGHTGRAY : GRAY;
            float block_w = pw / 4.0f;
            DrawRectangle((int)px_draw, (int)sy, (int)(block_w - 2), PLAT_H, fragCol);
            DrawRectangle((int)(px_draw + block_w), (int)sy + 10, (int)(block_w - 2), PLAT_H, fragCol);
            DrawRectangle((int)(px_draw + block_w * 2), (int)sy + 5, (int)(block_w - 2), PLAT_H, fragCol);
            DrawRectangle((int)(px_draw + block_w * 3), (int)sy + 15, (int)(block_w - 2), PLAT_H, fragCol);
            continue;
        }
        
        if (platforms[i].type == 0 || platforms[i].type == 1 || platforms[i].type == 5) {
            /* Brown body with colored top (Normais, Movem, Pequenas) */
            DrawRectangle((int)px_draw, (int)sy, (int)pw, PLAT_H, DARKBROWN);
            DrawRectangle((int)px_draw, (int)sy, (int)pw, 4, platNormalCol);
        } else if (platforms[i].type == 2 || platforms[i].type == 3) {
            /* Grey blocks (Pedras, Nuvens) */
            Color blockCol = (platforms[i].type == 2) ? LIGHTGRAY : GRAY;
            float block_w = pw / 4.0f;
            for (int b = 0; b < 4; b++) {
                DrawRectangle((int)(px_draw + b * block_w), (int)sy, (int)(block_w - 2), PLAT_H, blockCol);
            }
        } else if (platforms[i].type == 4) {
            /* Cogumelos (Red mushroom-style) */
            DrawRectangle((int)px_draw, (int)sy, (int)pw, PLAT_H, RED);
            DrawRectangle((int)(px_draw + pw * 0.15f), (int)(sy + 2), 4, 4, WHITE);
            DrawRectangle((int)(px_draw + pw * 0.45f), (int)(sy + 4), 3, 3, WHITE);
            DrawRectangle((int)(px_draw + pw * 0.75f), (int)(sy + 1), 5, 5, WHITE);
        }
    }

    /* player */
    dj_draw_player(pPlayerX, pPlayerY, dj_skin, dj_facing_right, dj_dead, 1.0f, pVelY, false);

    /* HUD */
    DrawText(TextFormat(L("Pontos: %d", "Score: %d"), dj_score), 10, 10, 20, WHITE);
    const char *hs_text = TextFormat(L("Recorde: %d", "Highscore: %d"), dj_highscore);
    DrawText(hs_text, DW - MeasureText(hs_text, 20) - 10, 10, 20, WHITE);



    if (dj_dead) {
        if (dj_store) {
            DrawRectangle(0, 0, DW, DH, BLACK);
            DrawText(L("LOJA", "SHOP"), (DW - MeasureText(L("LOJA", "SHOP"), 40)) / 2, 40, 40, WHITE);
            DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), dj_highscore), (DW - MeasureText(TextFormat(L("Recorde: %d", "Highscore: %d"), dj_highscore), 20)) / 2, 90, 20, GRAY);

            int thresholds[] = {0, 400, 800, 1300, 2000, 2800, 3800, 4800, 6000};
            for (int i = 0; i < 9; i++) {
                int row = i / 3, col = i % 3;
                int x = 60 + col * 110, y = 160 + row * 110;

                if (dj_selection == i) {
                    float pulse = 0.5f + 0.5f * sinf(gameTime * 6.0f);
                    DrawRectangle(x - 10, y - 10, 60, 60, (Color){200, 200, 200, (unsigned char)(60 + 40 * pulse)});
                }

                if (dj_highscore >= thresholds[i]) {
                    dj_draw_player((float)x + 5, (float)y + 5, i, 1, false, 2.0f, 0, true);
                    DrawText(TextFormat("%d", i+1), x - 25, y + 10, 20, WHITE);
                } else {
                    DrawRectangle(x, y, 40, 40, (Color){30, 30, 30, 255});
                    DrawText("?", x + 15, y + 10, 20, GRAY);
                    DrawText(TextFormat("%d", thresholds[i]), x, y + 45, 15, RED);
                }
            }
            DrawText(L("SETAS para navegar | ENTER para selecionar e sair", "ARROWS to navigate | ENTER to select and exit"), (DW - MeasureText(L("SETAS para navegar | ENTER para selecionar e sair", "ARROWS to navigate | ENTER to select and exit"), 16)) / 2, DH - 45, 16, GRAY);
        } else {
            DrawRectangle(0, 0, DW, DH, (Color){0, 0, 0, 200});
            int center_y = DH / 2 - 80;
            
            const char *msg = L("FIM DE JOGO", "GAME OVER");
            int tw = MeasureText(msg, 40);
            DrawText(msg, (DW - tw) / 2, center_y, 40, RED);
            
            DrawText(TextFormat(L("Pontos: %d", "Score: %d"), dj_score), (DW - MeasureText(TextFormat(L("Pontos: %d", "Score: %d"), dj_score), 24)) / 2, center_y + 60, 24, WHITE);
            DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), dj_highscore), (DW - MeasureText(TextFormat(L("Recorde: %d", "Highscore: %d"), dj_highscore), 18)) / 2, center_y + 90, 18, GRAY);
            
            float pulse = 0.5f + 0.5f * sinf(gameTime * 5.0f);
            Color pulse_c = (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)};
            DrawText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), (DW - MeasureText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), 18)) / 2, center_y + 130, 18, pulse_c);
            DrawText(L("Pressione 'S' para Loja", "Press 'S' for Shop"), (DW - MeasureText(L("Pressione 'S' para Loja", "Press 'S' for Shop"), 18)) / 2, center_y + 160, 18, pulse_c);
        }
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
        DrawText(cd_text, (DW - scaled_w) / 2, (DH - scaled_size) / 2, scaled_size, cd_col);
    }
}

void crazyjump_unload(void) { 
    if (sound_loaded) UnloadSound(pop_sound);
    printf("[CRAZYJUMP] Game unloaded\n"); 
}

Game CRAZYJUMP_GAME = {
    .name = "Crazy jump",
    .description = "Pule alto e desvie de plataformas falsas",
    .description_en = "Jump high and avoid fake platforms",
    .game_width = DW, .game_height = DH,
    .init = crazyjump_init, .update = crazyjump_update,
    .draw = crazyjump_draw, .unload = crazyjump_unload,
};
