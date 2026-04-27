/**
 * @file spaceinvaders.c
 * @brief Space Invaders clone — rectangles only, Chub style.
 * Arrows/AD move, Space shoots.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib/raylib.h"
#include "game.h"
#include "hub.h"

static int cd_num;
static float cd_timer;

#define IW 640
#define IH 480

#define PLAYER_W 40
#define PLAYER_H 16
#define PLAYER_SPEED 280.0f

#define ENEMY_COLS 10
#define ENEMY_ROWS 5
#define ENEMY_COUNT (ENEMY_COLS*ENEMY_ROWS)
#define ENEMY_W 36
#define ENEMY_H 22
#define ENEMY_GAP_X 12
#define ENEMY_GAP_Y 10

#define MAX_BULLETS 40
#define BULLET_W 4
#define BULLET_H 14
#define BULLET_SPEED 450.0f

#define OBSTACLE_COUNT 4
#define OBS_PIECE_W 8
#define OBS_PIECE_H 8
#define OBS_COLS 6
#define OBS_ROWS 4

typedef struct { float x, y; int active; float vx, vy; int power; } Bullet;
typedef struct { float x, y; int hp; int points; int type; float state_timer; float spawn_timer; float hit_timer; float death_timer; } Enemy;
typedef struct { float x, y; int type; bool active; } PowerUp;
typedef struct { int structure[OBS_ROWS][OBS_COLS]; float x, y; } Obstacle;

#define MAX_POWERUPS 5
static PowerUp powerups[MAX_POWERUPS];
static float double_shot_timer = 0;
static float freeze_timer = 0;
static float flash_ship_timer = 0;
static float flash_shield_timer = 0;
static float boss_spawn_timer = 0;
static float shield_regen_timer = 0;
static float player_shoot_timer = 0;

static Sound pop_sound;

static float px, py;
static int score, lives;
static int level;
static int highscore;
static Enemy enemies[ENEMY_COUNT];
static Bullet bullets[MAX_BULLETS];
static Obstacle obstacles[OBSTACLE_COUNT];
static float enemy_speed_x;
static int enemy_dir;
static float enemy_shoot_timer;
static Bullet enemy_bullets[MAX_BULLETS];
static int game_over, game_won;

static void inv_reset_enemies(void) {
    for (int i = 0; i < ENEMY_COUNT; i++) enemies[i].hp = 0;
    if (level % 5 != 0) {
        int total_w = ENEMY_COLS * (ENEMY_W + ENEMY_GAP_X) - ENEMY_GAP_X;
        int start_x = (IW - total_w) / 2;
        int start_y = 60;
        for (int i = 0; i < ENEMY_COUNT; i++) {
            int r = i / ENEMY_COLS, c = i % ENEMY_COLS;
            bool spawn = false;
            switch(level) {
                case 1: spawn = true; break;
                case 2: spawn = (abs(c - 4) <= r); break;
                case 3: spawn = (abs(c - 4) + abs(r - 2) <= 3); break;
                case 4: spawn = (c < 2 || c > 7); break;
                case 6: spawn = (r % 2 == 0); break;
                case 7: spawn = (c % 2 == 0); break;
                case 8: spawn = ((r+c) % 2 == 0); break;
                case 9: spawn = (r == 0 || r == 4 || c == 0 || c == 9); break;
                case 11: spawn = (r == 2 || c == 4); break;
                case 12: spawn = (abs(c - 4) == abs(r - 2)); break;
                case 13: spawn = (r < 3 && c > 2 && c < 7); break;
                case 14: spawn = (r >= 2 && (c == 2 || c == 7)); break;
                case 16: spawn = (r % 2 != c % 2); break;
                case 17: spawn = (c == 0 || c == 3 || c == 6 || c == 9); break;
                case 18: spawn = (r == c || r == (9-c)); break;
                case 19: spawn = (i % 3 == 0); break;
                default: spawn = (i < 30); break;
            }
            if (spawn) {
                enemies[i].x = (float)start_x + c * (ENEMY_W + ENEMY_GAP_X);
                enemies[i].y = (float)start_y + r * (ENEMY_H + ENEMY_GAP_Y);
                enemies[i].hp = (r == 0 ? 2 : 1); // Red aliens (Row 0) need 2 shots
                enemies[i].points = (ENEMY_ROWS - r) * 10;
                enemies[i].state_timer = 0;
                enemies[i].spawn_timer = 0.5f;
                enemies[i].hit_timer = 0;
                enemies[i].death_timer = 0;
                if (level > 15) enemies[i].type = GetRandomValue(0, 3);
                else if (level > 10) enemies[i].type = GetRandomValue(0, 2);
                else if (level > 5) enemies[i].type = GetRandomValue(0, 1);
                else enemies[i].type = 0;
            }
        }
        enemy_speed_x = 30.0f + (level * 10.0f);
    } else {
        int bidx = level / 5;
        float s = 1.0f + (bidx - 1) * 0.5f;
        enemies[0].x = (float)IW / 2 - (40 * s); enemies[0].y = 80;
        enemies[0].hp = 30 * bidx; enemies[0].points = 1000 * bidx;
        enemies[0].type = 4; enemy_speed_x = 80.0f + (bidx * 20.0f);
        enemies[0].spawn_timer = 1.0f; enemies[0].hit_timer = 0; enemies[0].death_timer = 0;
    }
    enemy_dir = 1;
}

static void inv_reset_obstacles(void) {
    int obs_full_w = OBS_COLS * 8; int gap = 80;
    int total_w = OBSTACLE_COUNT * obs_full_w + (OBSTACLE_COUNT - 1) * gap;
    int start_x = (IW - total_w) / 2;
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        obstacles[i].x = (float)start_x + i * (obs_full_w + gap); obstacles[i].y = py - 60;
        for (int r = 0; r < OBS_ROWS; r++) for (int c = 0; c < OBS_COLS; c++) obstacles[i].structure[r][c] = 1;
    }
}

static void inv_reset(void) {
    px = IW/2 - PLAYER_W/2; py = IH - PLAYER_H - 20;
    score = 0; highscore = hub_load_score(3); lives = 3;
    game_over = 0; game_won = 0; enemy_shoot_timer = 0; level = 1;
    double_shot_timer = 0; freeze_timer = 0; flash_ship_timer = 0; flash_shield_timer = 0;
    boss_spawn_timer = 0; shield_regen_timer = 0; player_shoot_timer = 0;
    inv_reset_enemies();
    for (int i = 0; i < MAX_BULLETS; i++) { bullets[i].active = 0; enemy_bullets[i].active = 0; }
    for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;
    inv_reset_obstacles();
}

static void inv_next_level(void) {
    level++; inv_reset_enemies();
    for (int i = 0; i < MAX_BULLETS; i++) { bullets[i].active = 0; enemy_bullets[i].active = 0; }
}

void spaceinvaders_init(void) { 
    cd_num = 3; cd_timer = 0.0f; inv_reset(); 
    pop_sound = LoadSound("assets/shared/pop.wav");
    printf("[SPACEINVADERS] Game initialized\n"); 
}

void spaceinvaders_update(void) {
    if (cd_num > 0) { cd_timer += GetFrameTime(); cd_num = 3 - (int)cd_timer; if (cd_num > 0) return; }
    float dt = GetFrameTime(); if (dt > 0.05f) dt = 0.05f;
    if (game_over || game_won) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (score > highscore) { highscore = score; hub_save_score(3, highscore); }
            inv_reset(); cd_num = 3; cd_timer = 0.0f;
        }
        return;
    }
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) px -= PLAYER_SPEED * dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) px += PLAYER_SPEED * dt;
    px = fmaxf(0, fminf(px, IW - PLAYER_W));

    if (player_shoot_timer > 0) player_shoot_timer -= dt;
    if ((IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) && player_shoot_timer <= 0) {
        player_shoot_timer = 0.25f; // Fixed cadence
        int sc = (double_shot_timer > 0) ? 2 : 1;
        for (int k = 0; k < sc; k++) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = 1; bullets[i].x = px + PLAYER_W/2 - BULLET_W/2 + (sc == 2 ? (k==0?-15:15) : 0);
                    bullets[i].y = py - BULLET_H; 
                    SetSoundPitch(pop_sound, 2.0f); SetSoundVolume(pop_sound, 0.3f);
                    PlaySound(pop_sound);
                    break;
                }
            }
        }
    }

    if (double_shot_timer > 0) double_shot_timer -= dt;
    if (freeze_timer > 0) freeze_timer -= dt;
    if (flash_ship_timer > 0) flash_ship_timer -= dt;
    if (flash_shield_timer > 0) flash_shield_timer -= dt;

    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (enemies[i].spawn_timer > 0) enemies[i].spawn_timer -= dt;
        if (enemies[i].hit_timer > 0) enemies[i].hit_timer -= dt;
        if (enemies[i].death_timer > 0) enemies[i].death_timer -= dt;
    }

    // Shield Regeneration (Very slow)
    shield_regen_timer += dt;
    if (shield_regen_timer > 8.0f) { // Every 8 seconds
        shield_regen_timer = 0;
        int o = GetRandomValue(0, OBSTACLE_COUNT-1);
        int r = GetRandomValue(0, OBS_ROWS-1);
        int c = GetRandomValue(0, OBS_COLS-1);
        obstacles[o].structure[r][c] = 1; // Regenerate one piece
    }

    // Boss Minion Spawn
    if (level % 5 == 0 && !game_over && !game_won) {
        boss_spawn_timer += dt;
        if (boss_spawn_timer > 10.0f) { // Every 10 seconds (Spawn less)
            boss_spawn_timer = 0;
            for (int i = 1; i < ENEMY_COUNT; i++) {
                if (enemies[i].hp <= 0 && enemies[i].death_timer <= 0) {
                    enemies[i].x = (float)GetRandomValue(50, IW - 50 - ENEMY_W);
                    enemies[i].y = 100;
                    enemies[i].hp = 1; enemies[i].type = 5; // Type 5: Mini Saucer Minion
                    enemies[i].points = 50;
                    enemies[i].spawn_timer = 0.5f; enemies[i].hit_timer = 0; enemies[i].death_timer = 0;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) { bullets[i].y -= BULLET_SPEED * dt; if (bullets[i].y < -BULLET_H) bullets[i].active = 0; }
        if (enemy_bullets[i].active) {
            enemy_bullets[i].x += enemy_bullets[i].vx * dt; enemy_bullets[i].y += enemy_bullets[i].vy * dt;
            if (enemy_bullets[i].y > IH || enemy_bullets[i].x < 0 || enemy_bullets[i].x > IW) enemy_bullets[i].active = 0;
        }
    }

    if (freeze_timer <= 0) {
        int edge = 0; int ac = 0;
        for (int i = 0; i < ENEMY_COUNT; i++) if (enemies[i].hp > 0) ac++;
        float cur_v = enemy_speed_x + (ENEMY_COUNT - ac) * 3.0f;
        for (int i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i].hp <= 0) continue;
            enemies[i].x += cur_v * enemy_dir * dt;
            float w = (enemies[i].type == 4) ? 80 * (1.0f + (level/5-1)*0.5f) : ENEMY_W;
            if (enemies[i].x < 5 || enemies[i].x + w > IW - 5) edge = 1;
        }
        if (edge) {
            enemy_dir *= -1;
            for (int i = 0; i < ENEMY_COUNT; i++) if (enemies[i].hp > 0 && enemies[i].type != 4) enemies[i].y += 12;
            enemy_speed_x += 2.0f;
        }
    }

    enemy_shoot_timer += dt;
    float fr = fmaxf(0.3f, (level % 5 == 0 ? 1.0f : 1.2f) - (level * 0.04f)); 
    if (level == 10) fr -= 0.15f; // Increase Second Boss fire rate
    if (level == 15) fr += 0.25f; // Decrease Third Boss fire rate
    
    for(int i=0; i<ENEMY_COUNT; i++) if (enemies[i].hp > 0 && enemies[i].type == 2) { enemies[i].state_timer += dt; if (enemies[i].state_timer > 3.0f) enemies[i].state_timer = 0; }
    
    if (enemy_shoot_timer > fr && freeze_timer <= 0) { // No shooting while frozen
        enemy_shoot_timer = 0;
        int al[ENEMY_COUNT], alc = 0;
        for (int i = 0; i < ENEMY_COUNT; i++) if (enemies[i].hp > 0 && enemies[i].type != 5 && enemies[i].spawn_timer <= 0) al[alc++] = i; // Minions (Type 5) don't shoot, and no shoot during spawn
        if (alc > 0) {
            Enemy *e = &enemies[al[GetRandomValue(0, alc-1)]];
            int bc = (e->type == 3) ? 3 : 1;
            if (e->type == 4) {
                bc = (level/5) + 1; 
                if (level == 10) bc = 2; // Nerf Level 10 Boss bullet count
            }
            for (int k = 0; k < bc; k++) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!enemy_bullets[i].active) {
                        enemy_bullets[i].active = 1;
                        enemy_bullets[i].x = e->x + ENEMY_W/2; enemy_bullets[i].y = e->y + ENEMY_H;
                        enemy_bullets[i].vy = BULLET_SPEED * (e->type == 4 ? 0.8f : 0.6f);
                        enemy_bullets[i].vx = (e->type == 3 ? (k-1)*100.0f : 0);
                        enemy_bullets[i].power = (e->type == 1 ? 2 : 1);
                        if (e->type == 4) {
                            float s = 1.0f + (level/5-1)*0.5f;
                            enemy_bullets[i].x = e->x + (40*s) + GetRandomValue(-80, 80);
                            enemy_bullets[i].y = e->y + (60*s);
                            enemy_bullets[i].vx = (float)GetRandomValue(-150, 150);
                        }
                        SetSoundPitch(pop_sound, 1.8f); SetSoundVolume(pop_sound, 0.2f);
                        PlaySound(pop_sound);
                        break;
                    }
                }
            }
        }
    }

    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active) continue;
        for (int e = 0; e < ENEMY_COUNT; e++) {
            if (enemies[e].hp <= 0) continue;
            
            // Shield Logic: Blocker (Type 2) or Protected by neighbor Blocker
            bool shielded = false;
            if (freeze_timer <= 0) {
                if (enemies[e].type == 2 && enemies[e].state_timer > 1.5f) shielded = true;
                else {
                    int r = e / ENEMY_COLS, c = e % ENEMY_COLS;
                    if (c > 0 && enemies[e-1].hp > 0 && enemies[e-1].type == 2 && enemies[e-1].state_timer > 1.5f) shielded = true;
                    if (c < ENEMY_COLS-1 && enemies[e+1].hp > 0 && enemies[e+1].type == 2 && enemies[e+1].state_timer > 1.5f) shielded = true;
                }
            }

            if (shielded) continue;

            float s = (enemies[e].type == 4) ? (1.0f + (level/5-1)*0.5f) : 1.0f;
            if (enemies[e].spawn_timer <= 0 && CheckCollisionRecs((Rectangle){bullets[b].x, bullets[b].y, BULLET_W, BULLET_H}, (Rectangle){enemies[e].x, enemies[e].y, (enemies[e].type==4?80*s:ENEMY_W), (enemies[e].type==4?60*s:ENEMY_H)})) {
                enemies[e].hp--; bullets[b].active = 0;
                enemies[e].hit_timer = 0.1f;
                score += enemies[e].points; if (score > highscore) { highscore = score; hub_save_score(3, highscore); }
                if (enemies[e].hp <= 0) {
                    enemies[e].death_timer = 0.3f;
                    SetSoundPitch(pop_sound, 1.0f); SetSoundVolume(pop_sound, 1.0f);
                    PlaySound(pop_sound);
                    bool drop = (GetRandomValue(1, 20) == 1) || (enemies[e].type == 5); 
                    if (drop) {
                        for(int p=0; p<MAX_POWERUPS; p++) if (!powerups[p].active) { powerups[p].active = true; powerups[p].x = enemies[e].x; powerups[p].y = enemies[e].y; powerups[p].type = GetRandomValue(0, 3); break; }
                    }
                }
                break;
            }
        }
    }

    for (int b = 0; b < MAX_BULLETS; b++) {
        for (int o = 0; o < OBSTACLE_COUNT; o++) {
            for (int r = 0; r < OBS_ROWS; r++) {
                for (int c = 0; c < OBS_COLS; c++) {
                    if (!obstacles[o].structure[r][c]) continue;
                    Rectangle pr = {obstacles[o].x + c*8, obstacles[o].y + r*8, 8, 8};
                    if (bullets[b].active && CheckCollisionRecs((Rectangle){bullets[b].x, bullets[b].y, BULLET_W, BULLET_H}, pr)) { obstacles[o].structure[r][c] = 0; bullets[b].active = 0; }
                    if (enemy_bullets[b].active && CheckCollisionRecs((Rectangle){enemy_bullets[b].x-2, enemy_bullets[b].y, (enemy_bullets[b].power>1?8:4), BULLET_H}, pr)) { obstacles[o].structure[r][c] = 0; enemy_bullets[b].active = 0; }
                }
            }
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!enemy_bullets[i].active) continue;
        int bw = (enemy_bullets[i].power > 1) ? 8 : 4;
        if (CheckCollisionRecs((Rectangle){enemy_bullets[i].x - bw/2, enemy_bullets[i].y, bw, BULLET_H}, (Rectangle){px, py, PLAYER_W, PLAYER_H})) {
            enemy_bullets[i].active = 0; lives -= enemy_bullets[i].power; // Orange bullets deal power=2 damage
            if (lives <= 0) game_over = 1;
        }
    }

    for (int i = 0; i < ENEMY_COUNT; i++) {
        float h = (enemies[i].type == 4) ? 60 * (1.0f + (level/5-1)*0.5f) : ENEMY_H;
        if (enemies[i].hp > 0 && enemies[i].y + h >= py) game_over = 1;
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) continue;
        powerups[i].y += 150.0f * dt; if (powerups[i].y > IH) powerups[i].active = false;
        if (CheckCollisionRecs((Rectangle){powerups[i].x, powerups[i].y, 24, 24}, (Rectangle){px, py, PLAYER_W, PLAYER_H})) {
            powerups[i].active = false;
            switch(powerups[i].type) { case 0: double_shot_timer = 20.0f; break; case 1: lives++; flash_ship_timer = 0.5f; break; case 2: inv_reset_obstacles(); flash_shield_timer = 0.5f; break; case 3: freeze_timer = 8.0f; break; } // Less duration
        }
    }

    int all_dead = 1;
    for (int i = 0; i < ENEMY_COUNT; i++) if (enemies[i].hp > 0) { all_dead = 0; break; }
    if (all_dead) { if (level < 20) inv_next_level(); else game_won = 1; }
}

void spaceinvaders_draw(void) {
    ClearBackground((Color){5,5,15,255});
    srand(123); for (int i = 0; i < 40; i++) DrawRectangle(rand()%IW, rand()%IH, 2, 2, (Color){255,255,255,(unsigned char)(60+rand()%80)});
    srand((unsigned)GetTime());

    Color row_cols[] = { {255,80,80,255}, {255,140,60,255}, {255,200,50,255}, {80,220,255,255}, {180,100,255,255} };

    for (int o = 0; o < OBSTACLE_COUNT; o++)
        for (int r = 0; r < OBS_ROWS; r++)
            for (int c = 0; c < OBS_COLS; c++)
                if (obstacles[o].structure[r][c]) DrawRectangle((int)(obstacles[o].x + c*8), (int)(obstacles[o].y + r*8), 8, 8, (flash_shield_timer > 0 ? WHITE : GREEN));

    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (enemies[i].hp <= 0 && enemies[i].death_timer <= 0) continue;
        int ex = (int)enemies[i].x, ey = (int)enemies[i].y;
        
        Color base_c;
        if (freeze_timer > 0) base_c = BLUE;
        else {
            switch(enemies[i].type) {
                case 1: base_c = ORANGE; break;
                case 2: base_c = (enemies[i].state_timer > 1.5f) ? SKYBLUE : DARKBLUE; break;
                case 3: base_c = (Color){0, 255, 100, 255}; break;
                case 4: base_c = PURPLE; break;
                case 5: base_c = PURPLE; break;
                default: base_c = row_cols[(i/10)%5]; break;
            }
        }

        if (enemies[i].spawn_timer > 0) {
            int size = 0;
            float target_w = (enemies[i].type == 4) ? (80.0f * (1.0f + (level/5-1)*0.5f)) : (float)ENEMY_W;
            
            if (enemies[i].spawn_timer > 0.30f) size = (int)(target_w / 4);
            else if (enemies[i].spawn_timer > 0.05f) size = (int)(target_w / 2);
            else { /* No square */ }
            
            if (size > 0) {
                DrawRectangle(ex + (int)target_w/2 - size/2, ey + ENEMY_H/2 - size/2, size, size, base_c);
                continue;
            }
        }
        if (enemies[i].death_timer > 0) {
            int size = 0;
            float target_w = (enemies[i].type == 4) ? (80.0f * (1.0f + (level/5-1)*0.5f)) : (float)ENEMY_W;

            if (enemies[i].death_timer > 0.15f) size = (int)(target_w / 2);
            else size = (int)(target_w / 4);

            DrawRectangle(ex + (int)target_w/2 - size/2, ey + ENEMY_H/2 - size/2, size, size, base_c);
            continue;
        }

        int bidx = level / 5;
        if (enemies[i].type == 4) {
            float s = 1.0f + (bidx - 1) * 0.5f; 
            Color b1 = DARKPURPLE, b2 = PURPLE, b3 = PINK, b4 = YELLOW;
            if (freeze_timer > 0) { b1 = DARKBLUE; b2 = BLUE; b3 = SKYBLUE; b4 = SKYBLUE; }
            if (enemies[i].hit_timer > 0) { b1 = b2 = b3 = b4 = WHITE; }

            DrawRectangle(ex, ey + (int)(20*s), (int)(80*s), (int)(20*s), b1); 
            DrawRectangle(ex + (int)(10*s), ey, (int)(60*s), (int)(30*s), b2);     
            DrawRectangle(ex + (int)(20*s), ey - (int)(10*s), (int)(40*s), (int)(10*s), b3);  
            for(int j=0; j<4; j++) DrawRectangle(ex + (int)(5*s) + j*(int)(20*s), ey + (int)(25*s), (int)(10*s), (int)(5*s), b4);
            DrawRectangle(ex + (int)(30*s), ey + (int)(10*s), (int)(20*s), (int)(10*s), WHITE);
            DrawRectangle(ex + (int)(35*s) + (int)(sinf((float)GetTime()*5)*(10*s)), ey + (int)(12*s), (int)(6*s), (int)(6*s), BLACK);
            if (bidx >= 2) { DrawRectangle(ex - (int)(15*s), ey + (int)(10*s), (int)(15*s), (int)(30*s), b1); DrawRectangle(ex + (int)(80*s), ey + (int)(10*s), (int)(15*s), (int)(30*s), b1); }
            if (bidx >= 3) { for(int j=0; j<3; j++) DrawRectangle(ex + (int)((15+j*25)*s), ey + (int)(40*s), (int)(10*s), (int)(15*s), (freeze_timer > 0 ? SKYBLUE : VIOLET)); }
            if (bidx >= 4) { DrawRectangle(ex + (int)(5*s), ey - (int)(25*s), (int)(5*s), (int)(25*s), b2); DrawRectangle(ex + (int)(70*s), ey - (int)(25*s), (int)(5*s), (int)(25*s), b2); DrawRectangle(ex + (int)(5*s), ey + (int)(5*s), (int)(70*s), (int)(5*s), (freeze_timer > 0 ? SKYBLUE : (Color){200,150,255,255})); }
            continue;
        }

        int shape_type = (i/10)%5;
        Color ec = (enemies[i].hit_timer > 0) ? WHITE : base_c;
        DrawRectangle(ex, ey, ENEMY_W, ENEMY_H, ec);
        if (enemies[i].type == 5) { // Mini Saucer Visual
            DrawRectangle(ex + 4, ey - 4, ENEMY_W - 8, 4, PINK);
            DrawRectangle(ex - 4, ey + 8, ENEMY_W + 8, 6, ec);
            continue;
        }
        // Base Shape Features based on Row (shape_type)
        switch (shape_type) {
            case 0: // Row 0 (Red): Tall with thin antennas
                DrawRectangle(ex + ENEMY_W/2 - 2, ey - 6, 4, 6, ec);
                DrawRectangle(ex - 2, ey + 4, 2, 14, ec);
                DrawRectangle(ex + ENEMY_W, ey + 4, 2, 14, ec);
                break;
            case 1: // Row 1 (Orange): Wide with side-fins
                DrawRectangle(ex - 8, ey + 8, 8, 6, ec);
                DrawRectangle(ex + ENEMY_W, ey + 8, 8, 6, ec);
                break;
            case 2: // Row 2 (Yellow): Horned
                DrawRectangle(ex + 4, ey - 4, 6, 4, ec);
                DrawRectangle(ex + ENEMY_W - 10, ey - 4, 6, 4, ec);
                DrawRectangle(ex + 2, ey + ENEMY_H, 4, 4, ec);
                DrawRectangle(ex + ENEMY_W - 6, ey + ENEMY_H, 4, 4, ec);
                break;
            case 3: // Row 3 (Cyan): Pincer-like
                DrawRectangle(ex - 4, ey - 2, 4, 8, ec);
                DrawRectangle(ex + ENEMY_W, ey - 2, 4, 8, ec);
                DrawRectangle(ex + 6, ey + ENEMY_H, 4, 6, ec);
                DrawRectangle(ex + ENEMY_W - 10, ey + ENEMY_H, 4, 6, ec);
                break;
            case 4: // Row 4 (Purple): Diamond/Spiky
                DrawRectangle(ex + ENEMY_W/2 - 4, ey - 8, 8, 8, ec);
                DrawRectangle(ex + ENEMY_W/2 - 4, ey + ENEMY_H, 8, 8, ec);
                break;
        }

        // Type-specific appearance additions (Overrides/Adds to base shape)
        if (enemies[i].type == 1) { // Heavy: Extra side armor
            DrawRectangle(ex - 6, ey + 2, 6, 18, ec);
            DrawRectangle(ex + ENEMY_W, ey + 2, 6, 18, ec);
            DrawRectangle(ex + 4, ey + ENEMY_H, ENEMY_W - 8, 4, ec);
        }
        if (enemies[i].type == 2) { // Blocker: Side defenses (No white outline)
            DrawRectangle(ex - 4, ey + 4, 4, 14, ec);
            DrawRectangle(ex + ENEMY_W, ey + 4, 4, 14, ec);
        }
        if (enemies[i].type == 3) { // Spread: Triple cannons (Green)
            DrawRectangle(ex + 2, ey + ENEMY_H, 6, 10, ec);
            DrawRectangle(ex + ENEMY_W/2 - 3, ey + ENEMY_H, 6, 10, ec);
            DrawRectangle(ex + ENEMY_W - 8, ey + ENEMY_H, 6, 10, ec);
        }

        Color eyec = (enemies[i].type == 3 ? GREEN : WHITE);
        if (enemies[i].type == 2 && enemies[i].state_timer > 1.5f) eyec = YELLOW;
        if (freeze_timer > 0) eyec = SKYBLUE;
        DrawRectangle(ex + 8, ey + 6, 6, 6, eyec); DrawRectangle(ex + ENEMY_W - 14, ey + 6, 6, 6, eyec);
        DrawRectangle(ex + 10, ey + 8, 2, 2, (freeze_timer > 0 ? BLUE : BLACK)); DrawRectangle(ex + ENEMY_W - 12, ey + 8, 2, 2, (freeze_timer > 0 ? BLUE : BLACK));
    }

    Color pc = (flash_ship_timer > 0) ? RED : (Color){80,200,255,255};
    DrawRectangle((int)px, (int)py, PLAYER_W, PLAYER_H, pc);
    if (double_shot_timer > 0) { DrawRectangle((int)px+5, (int)py-6, 6, 6, pc); DrawRectangle((int)px+PLAYER_W-11, (int)py-6, 6, 6, pc); }
    else DrawRectangle((int)px+PLAYER_W/2-3, (int)py-6, 6, 6, pc);

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) DrawRectangle((int)bullets[i].x, (int)bullets[i].y, BULLET_W, BULLET_H, (Color){255,255,100,255});
        if (enemy_bullets[i].active) {
            int bw = (enemy_bullets[i].power > 1) ? 8 : 4;
            DrawRectangle((int)enemy_bullets[i].x - bw/2, (int)enemy_bullets[i].y, bw, BULLET_H, (enemy_bullets[i].power > 1 ? ORANGE : (Color){255,80,80,255}));
        }
    }
    for (int i = 0; i < MAX_POWERUPS; i++) if (powerups[i].active) { DrawRectangle((int)powerups[i].x, (int)powerups[i].y, 24, 24, PURPLE); DrawText("P", (int)powerups[i].x + 6, (int)powerups[i].y + 4, 15, WHITE); }

    DrawText(TextFormat(L("Pontos: %d", "Score: %d"), score), 10, 10, 20, WHITE);
    int world = (level - 1) / 5 + 1;
    int stage = (level - 1) % 5 + 1;

    // Boss Health Bar (Vertical on the Right)
    if (level % 5 == 0 && enemies[0].hp > 0 && enemies[0].type == 4) {
        int bidx = level / 5;
        float max_hp = 30.0f * bidx;
        float pct = (float)enemies[0].hp / max_hp;
        int bar_h = 250;
        int bar_w = 15;
        int bar_x = IW - 35;
        int bar_y = (IH - bar_h) / 2;
        
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){100, 0, 0, 255}); // Dark Red background
        int fill_h = (int)(bar_h * pct);
        DrawRectangle(bar_x, bar_y + (bar_h - fill_h), bar_w, fill_h, RED);
    }

    DrawText(TextFormat(L("Recorde: %d", "Highscore: %d"), highscore), IW - 160, 10, 20, WHITE);
    
    int lives_w = lives * 24 - 8;
    int lives_start_x = (IW - lives_w) / 2;
    for (int i = 0; i < lives; i++) DrawRectangle(lives_start_x + i * 24, 12, 16, 16, RED);

    const char* lvl_text = TextFormat("%d-%d", world, stage);
    DrawText(lvl_text, (IW - MeasureText(lvl_text, 20)) / 2, 35, 20, YELLOW);



    if (game_over || game_won) {
        DrawRectangle(0, 0, IW, IH, BLACK);
        if (game_over) DrawText(L("FIM DE JOGO", "GAME OVER"), (IW - MeasureText(L("FIM DE JOGO", "GAME OVER"), 50)) / 2, IH/2-80, 50, RED);
        else DrawText(L("VITORIA!", "VICTORY!"), (IW - MeasureText(L("VITORIA!", "VICTORY!"), 50)) / 2, IH/2-80, 50, GOLD);
        
        DrawText(TextFormat(L("Nivel: %d-%d", "Level: %d-%d"), world, stage), (IW - MeasureText(TextFormat(L("Nivel: %d-%d", "Level: %d-%d"), world, stage), 25)) / 2, IH/2-20, 25, YELLOW);
        DrawText(TextFormat(L("Pontos: %d", "Score: %d"), score), (IW - MeasureText(TextFormat(L("Pontos: %d", "Score: %d"), score), 30)) / 2, IH/2+30, 30, WHITE);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        Color pulse_c = (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)};
        DrawText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), (IW - MeasureText(L("Pressione ENTER para Reiniciar", "Press ENTER to Restart"), 18)) / 2, IH/2+90, 18, pulse_c);
    }
    if (cd_num > 0) { char t[4]; snprintf(t, 4, "%d", cd_num); DrawText(t, (IW - MeasureText(t, 100)) / 2, (IH-100)/2, 100, (Color){255,255,200,200}); }
}

void spaceinvaders_unload(void) { 
    UnloadSound(pop_sound);
    printf("[SPACEINVADERS] Game unloaded\n"); 
}

Game SPACEINVADERS_GAME = { .name = "spaCe invaders", .description = "Defenda a Terra da invasao alienigena", .description_en = "Defend Earth from the alien invasion", .game_width = IW, .game_height = IH, .init = spaceinvaders_init, .update = spaceinvaders_update, .draw = spaceinvaders_draw, .unload = spaceinvaders_unload, };
