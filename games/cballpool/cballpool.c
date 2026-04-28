/**
 * @file cballpool.c
 * @brief 8-Ball Pool — simplified with square balls, Chub style.
 * Click & drag to aim (slingshot style), release to shoot.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib/raylib.h"
#include "game.h"
#include "hub.h"

#define PW 640
#define PH 400

#define BALL_SIZE 12
#define BALL_COUNT 16
#define POCKET_SIZE 22
#define POCKET_COUNT 6
#define TABLE_X 40
#define TABLE_Y 40
#define TABLE_W (PW-80)
#define TABLE_H (PH-80)
#define FRICTION 0.985f
#define MAX_POWER 200.0f

typedef struct {
    float x, y, vx, vy;
    int pocketed;
    Color color;
    int number; /* 0=cue */
} Ball;

static Ball balls[BALL_COUNT];
static Vector2 pockets[POCKET_COUNT];
static int dragging;
static float drag_start_x, drag_start_y;
static float drag_current_x, drag_current_y;
static int cd_num;
static float cd_timer;

static int p1_score, p2_score, turn;
static int cballpool_state; /* -1=menu 0=aiming 1=moving 2=gameover */
static int game_mode; /* 0=CPU 1=PVP */
static int cue_pocketed;

static int cpu_thinking = 0;
static Sound pop_sound;
static bool sound_loaded = false;
static float cpu_timer = 0.0f;
static float cpu_power = 0.0f;
static float cpu_nx = 0.0f, cpu_ny = 0.0f;

static int p1_type; /* 0=none, 1=solids, 2=stripes */
static int p2_type;
static int winner; /* 0=none, 1=P1, 2=P2 */
static int turn_scored;
static int turn_foul;

static Color ball_colors[] = {
    WHITE, /* 0: cue */
    {255,220,50,255},{80,120,255,255},{255,80,80,255},{160,50,200,255},
    {255,140,40,255},{50,180,80,255},{180,50,80,255},{20,20,20,255}, /* 8 ball */
    {255,220,50,200},{80,120,255,200},{255,80,80,200},{160,50,200,200},
    {255,140,40,200},{50,180,80,200},{180,50,80,200}
};

static void setup_balls(void) {
    /* Strictly intercalated rack pattern (S, T, S, T, 8, S, T, S, T...) */
    int pattern[15] = {1, 9, 2, 10, 8, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15};
    
    /* triangle rack */
    float start_x = TABLE_X + TABLE_W * 0.75f;
    float start_y = TABLE_Y + TABLE_H / 2.0f;
    int p_idx = 0;
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row <= col; row++) {
            int ball_idx = pattern[p_idx];
            balls[ball_idx].x = start_x + col * (BALL_SIZE + 1);
            balls[ball_idx].y = start_y - (col * (BALL_SIZE + 1) / 2.0f) + row * (BALL_SIZE + 1);
            balls[ball_idx].vx = 0; balls[ball_idx].vy = 0;
            balls[ball_idx].pocketed = 0;
            balls[ball_idx].color = ball_colors[ball_idx];
            balls[ball_idx].number = ball_idx;
            p_idx++;
        }
    }
    /* cue ball */
    balls[0].x = TABLE_X + TABLE_W * 0.25f;
    balls[0].y = TABLE_Y + TABLE_H / 2.0f;
    balls[0].vx = 0; balls[0].vy = 0;
    balls[0].pocketed = 0;
    balls[0].color = WHITE;
    balls[0].number = 0;
}

static void cballpool_reset(void) {
    setup_balls();
    p1_score = 0; p2_score = 0; turn = 1;
    cballpool_state = -1; dragging = 0; cue_pocketed = 0;
    cd_num = 0;
    cpu_thinking = 0; cpu_timer = 0.0f;
    p1_type = 0; p2_type = 0; winner = 0;
    turn_scored = 0; turn_foul = 0;

    /* pockets */
    pockets[0] = (Vector2){TABLE_X, TABLE_Y};
    pockets[1] = (Vector2){TABLE_X + TABLE_W/2, TABLE_Y};
    pockets[2] = (Vector2){TABLE_X + TABLE_W, TABLE_Y};
    pockets[3] = (Vector2){TABLE_X, TABLE_Y + TABLE_H};
    pockets[4] = (Vector2){TABLE_X + TABLE_W/2, TABLE_Y + TABLE_H};
    pockets[5] = (Vector2){TABLE_X + TABLE_W, TABLE_Y + TABLE_H};
}

void cballpool_init(void) { 
    cballpool_reset(); 
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;
    printf("[CBALLPOOL] Game initialized\n"); 
}

static int balls_moving(void) {
    for (int i = 0; i < BALL_COUNT; i++) {
        if (balls[i].pocketed) continue;
        if (fabsf(balls[i].vx) > 0.5f || fabsf(balls[i].vy) > 0.5f) return 1;
    }
    return 0;
}

void cballpool_update(void) {
    if (cballpool_state == -1) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) { game_mode = 0; cballpool_state = 0; cd_num = 3; cd_timer = 0; }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { game_mode = 1; cballpool_state = 0; cd_num = 3; cd_timer = 0; }
        
        Vector2 mp = GetMousePosition();
        Rectangle r1 = { PW / 2 - 260, 240, 240, 40 };
        Rectangle r2 = { PW / 2 + 40, 240, 200, 40 };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mp, r1)) { game_mode = 0; cballpool_state = 0; cd_num = 3; cd_timer = 0; }
            if (CheckCollisionPointRec(mp, r2)) { game_mode = 1; cballpool_state = 0; cd_num = 3; cd_timer = 0; }
        }
        return;
    }

    if (cd_num > 0) {
        cd_timer += GetFrameTime();
        cd_num = 3 - (int)cd_timer;
        if (cd_num > 0) return;
    }

    float dt = GetFrameTime();
    if (dt > 0.03f) dt = 0.03f;

    if (cballpool_state == 2) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) { cballpool_reset(); cd_num = 3; cd_timer = 0.0f; }
        return;
    }

    if (cballpool_state == 0 && !balls_moving()) {
        /* re-spot cue if pocketed */
        if (cue_pocketed) {
            balls[0].x = TABLE_X + TABLE_W * 0.25f;
            balls[0].y = TABLE_Y + TABLE_H / 2.0f;
            balls[0].pocketed = 0;
            cue_pocketed = 0;
        }

        if (turn == 2 && game_mode == 0) {
            /* CPU logic */
            if (!cpu_thinking) {
                int best_target = -1;
                float best_score = -9999.0f;
                float target_nx = 0, target_ny = 0;

                int start_b = 1, end_b = 15;
                if (p2_type == 1) { start_b = 1; end_b = 7; }
                else if (p2_type == 2) { start_b = 9; end_b = 15; }
                
                int balls_rem = 0;
                for (int i=start_b; i<=end_b; i++) if (!balls[i].pocketed) balls_rem++;

                for (int i = 1; i <= 15; i++) {
                    // Skip if not player's type (unless it's the 8 ball and we are ready for it)
                    if (p2_type == 1 && i > 7 && i != 8) continue;
                    if (p2_type == 2 && i < 9 && i != 8) continue;
                    if (p2_type != 0 && i == 8 && balls_rem > 0) continue;
                    if (balls[i].pocketed) continue;

                    for (int p=0; p<POCKET_COUNT; p++) {
                        float bpx = pockets[p].x - balls[i].x;
                        float bpy = pockets[p].y - balls[i].y;
                        float dist_bp = sqrtf(bpx*bpx + bpy*bpy);
                        float bpx_n = bpx / dist_bp;
                        float bpy_n = bpy / dist_bp;

                        float hit_x = balls[i].x - bpx_n * BALL_SIZE;
                        float hit_y = balls[i].y - bpy_n * BALL_SIZE;

                        float c_hx = hit_x - balls[0].x;
                        float c_hy = hit_y - balls[0].y;
                        float dist_ch = sqrtf(c_hx*c_hx + c_hy*c_hy);
                        float c_hx_n = c_hx / dist_ch;
                        float c_hy_n = c_hy / dist_ch;

                        float dot = (c_hx_n * bpx_n) + (c_hy_n * bpy_n);
                        float score = dot * 1000.0f - dist_bp * 0.2f - dist_ch * 0.1f;
                        
                        if (score > best_score) {
                            best_score = score;
                            best_target = i;
                            target_nx = c_hx_n;
                            target_ny = c_hy_n;
                        }
                    }
                }

                if (best_target != -1) {
                    cpu_nx = target_nx;
                    cpu_ny = target_ny;
                    cpu_power = 120.0f + (GetRandomValue(0, 40));
                    cpu_thinking = 1;
                    cpu_timer = 0.0f;
                }
            } else {
                cpu_timer += dt;
                if (cpu_timer >= 1.4f) {
                    balls[0].vx = cpu_nx * cpu_power * 4.0f;
                    balls[0].vy = cpu_ny * cpu_power * 4.0f;
                    cballpool_state = 1;
                    cpu_thinking = 0;
                    turn_scored = 0; turn_foul = 0;
                }
            }
        } else {
            Vector2 mp = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                float dx = mp.x - balls[0].x;
                float dy = mp.y - balls[0].y;
                if (sqrtf(dx*dx+dy*dy) < BALL_SIZE*3) {
                    dragging = 1;
                    drag_start_x = mp.x;
                    drag_start_y = mp.y;
                    drag_current_x = mp.x;
                    drag_current_y = mp.y;
                }
            }
            if (dragging) {
                drag_current_x = mp.x;
                drag_current_y = mp.y;
            }
            if (dragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                float dx = drag_start_x - mp.x;
                float dy = drag_start_y - mp.y;
                float power = sqrtf(dx*dx+dy*dy);
                if (power > MAX_POWER) power = MAX_POWER;
                if (power > 5.0f) {
                    balls[0].vx = (dx/power) * power * 4.0f;
                    balls[0].vy = (dy/power) * power * 4.0f;
                    cballpool_state = 1;
                    turn_scored = 0; turn_foul = 0;
                }
                dragging = 0;
            }
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) dragging = 0;
        }
    }

    if (cballpool_state == 1) {
        /* physics update */
        for (int i = 0; i < BALL_COUNT; i++) {
            if (balls[i].pocketed) continue;
            balls[i].x += balls[i].vx * dt;
            balls[i].y += balls[i].vy * dt;
            balls[i].vx *= FRICTION;
            balls[i].vy *= FRICTION;

            /* wall collisions */
            if (balls[i].x < TABLE_X + BALL_SIZE/2) { balls[i].x = TABLE_X + BALL_SIZE/2; balls[i].vx *= -0.8f; }
            if (balls[i].x > TABLE_X + TABLE_W - BALL_SIZE/2) { balls[i].x = TABLE_X + TABLE_W - BALL_SIZE/2; balls[i].vx *= -0.8f; }
            if (balls[i].y < TABLE_Y + BALL_SIZE/2) { balls[i].y = TABLE_Y + BALL_SIZE/2; balls[i].vy *= -0.8f; }
            if (balls[i].y > TABLE_Y + TABLE_H - BALL_SIZE/2) { balls[i].y = TABLE_Y + TABLE_H - BALL_SIZE/2; balls[i].vy *= -0.8f; }
        }

        /* ball-ball collisions */
        for (int i = 0; i < BALL_COUNT; i++) {
            if (balls[i].pocketed) continue;
            for (int j = i+1; j < BALL_COUNT; j++) {
                if (balls[j].pocketed) continue;
                float dx = balls[j].x - balls[i].x;
                float dy = balls[j].y - balls[i].y;
                float dist = sqrtf(dx*dx+dy*dy);
                if (dist < BALL_SIZE) {
                    float nx = dx / dist;
                    float ny = dy / dist;
                    float tx = -ny;
                    float ty = nx;

                    float v1n = nx * balls[i].vx + ny * balls[i].vy;
                    float v1t = tx * balls[i].vx + ty * balls[i].vy;
                    float v2n = nx * balls[j].vx + ny * balls[j].vy;
                    float v2t = tx * balls[j].vx + ty * balls[j].vy;

                    balls[i].vx = v2n * nx + v1t * tx;
                    balls[i].vy = v2n * ny + v1t * ty;
                    balls[j].vx = v1n * nx + v2t * tx;
                    balls[j].vy = v1n * ny + v2t * ty;

                    if (sound_loaded) {
                        float vol = (fabsf(v1n) + fabsf(v2n)) / 20.0f;
                        if (vol > 1.0f) vol = 1.0f;
                        if (vol > 0.1f) {
                            SetSoundPitch(pop_sound, 0.8f + (GetRandomValue(0, 40) / 100.0f));
                            SetSoundVolume(pop_sound, vol);
                            PlaySound(pop_sound);
                        }
                    }

                    /* separate */
                    float overlap = BALL_SIZE - dist;
                    balls[i].x -= nx * overlap/2;
                    balls[i].y -= ny * overlap/2;
                    balls[j].x += nx * overlap/2;
                    balls[j].y += ny * overlap/2;
                }
            }
        }

        /* pocket check */
        for (int i = 0; i < BALL_COUNT; i++) {
            if (balls[i].pocketed) continue;
            for (int p = 0; p < POCKET_COUNT; p++) {
                float dx = balls[i].x - pockets[p].x;
                float dy = balls[i].y - pockets[p].y;
                if (sqrtf(dx*dx+dy*dy) < POCKET_SIZE) {
                    balls[i].pocketed = 1;
                    balls[i].vx = 0; balls[i].vy = 0;
                    if (i == 0) { 
                        cue_pocketed = 1; 
                        turn_foul = 1; 
                    }
                    else if (i == 8) { 
                        cballpool_state = 2; 
                        int player_type = (turn == 1) ? p1_type : p2_type;
                        int all_pocketed = 1;
                        if (player_type == 1) {
                            for(int b=1; b<=7; b++) if(!balls[b].pocketed) all_pocketed = 0;
                        } else if (player_type == 2) {
                            for(int b=9; b<=15; b++) if(!balls[b].pocketed) all_pocketed = 0;
                        } else {
                            all_pocketed = 0;
                        }
                        if (all_pocketed && !cue_pocketed) winner = turn;
                        else winner = (turn == 1) ? 2 : 1;
                    }
                    else {
                        int is_solid = (i >= 1 && i <= 7);
                        int b_type = is_solid ? 1 : 2;
                        if (p1_type == 0) {
                            if (turn == 1) {
                                p1_type = b_type;
                                p2_type = (b_type == 1) ? 2 : 1;
                            } else {
                                p2_type = b_type;
                                p1_type = (b_type == 1) ? 2 : 1;
                            }
                            turn_scored = 1;
                            if (turn == 1) p1_score++; else p2_score++;
                        } else {
                            int player_type = (turn == 1) ? p1_type : p2_type;
                            if (player_type == b_type) {
                                turn_scored = 1;
                                if (turn == 1) p1_score++; else p2_score++;
                            } else {
                                if (turn == 1) p2_score++; else p1_score++;
                            }
                        }
                    }
                }
            }
        }

        if (!balls_moving()) {
            if (cballpool_state != 2) {
                cballpool_state = 0;
                if (turn_foul || !turn_scored) {
                    turn = (turn == 1) ? 2 : 1;
                }
            }
        }
    }
}

void cballpool_draw(void) {
    ClearBackground((Color){15,20,15,255});

    if (cballpool_state == -1) {
        DrawRectangle(0,0,PW,PH,(Color){0,0,0,180});
        DrawText(L("SELECIONE O MODO", "SELECT MODE"), PW / 2 - MeasureText(L("SELECIONE O MODO", "SELECT MODE"), 40) / 2, 100, 40, WHITE);
        
        Vector2 mp = GetMousePosition();
        Rectangle r1 = { PW / 2 - 260, 240, 240, 40 };
        Rectangle r2 = { PW / 2 + 40, 240, 200, 40 };
        
        Color c1 = CheckCollisionPointRec(mp, r1) ? LIGHTGRAY : WHITE;
        Color c2 = CheckCollisionPointRec(mp, r2) ? LIGHTGRAY : WHITE;
        if (game_mode == 0) c1 = PURPLE;
        if (game_mode == 1) c2 = PURPLE;

        DrawText(L("<- 1 Jogador (Vs CPU)", "<- 1 Player (Vs CPU)"), PW / 2 - 250, 250, 20, c1);
        DrawText(L("2 Jogadores ->", "2 Players ->"), PW / 2 + 50, 250, 20, c2);
        DrawText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), PW / 2 - MeasureText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), 20) / 2, PH - 60, 20, WHITE);
        return;
    }

    /* table */
    DrawRectangle(TABLE_X-6, TABLE_Y-6, TABLE_W+12, TABLE_H+12, (Color){80,50,30,255});
    DrawRectangle(TABLE_X, TABLE_Y, TABLE_W, TABLE_H, (Color){30,110,50,255});

    /* pockets */
    for (int i = 0; i < POCKET_COUNT; i++) {
        // Border (Larger)
        DrawRectangle((int)pockets[i].x-POCKET_SIZE/2 - 4, (int)pockets[i].y-POCKET_SIZE/2 - 4, POCKET_SIZE + 8, POCKET_SIZE + 8, (Color){45,45,45,255});
        // Hole
        DrawRectangle((int)pockets[i].x-POCKET_SIZE/2, (int)pockets[i].y-POCKET_SIZE/2, POCKET_SIZE, POCKET_SIZE, (Color){15,15,15,255});
    }

    /* balls */
    for (int i = 0; i < BALL_COUNT; i++) {
        if (balls[i].pocketed) continue;
        int bs = BALL_SIZE;
        if (i == 0) {
            DrawRectangle((int)balls[i].x-bs/2, (int)balls[i].y-bs/2, bs, bs, balls[i].color);
        } else if (i == 8) {
            DrawRectangle((int)balls[i].x-bs/2, (int)balls[i].y-bs/2, bs, bs, balls[i].color);
            int tw = MeasureText("C", 8);
            DrawText("C", (int)balls[i].x - tw/2, (int)balls[i].y - 4, 8, WHITE);
        } else if (i >= 1 && i <= 7) {
            DrawRectangle((int)balls[i].x-bs/2, (int)balls[i].y-bs/2, bs, bs, balls[i].color);
        } else {
            DrawRectangle((int)balls[i].x-bs/2, (int)balls[i].y-bs/2, bs, bs, balls[i].color);
            DrawRectangle((int)balls[i].x-3, (int)balls[i].y-3, 6, 6, WHITE);
        }
    }

    /* cue line & aiming */
    if (cballpool_state == 0 && !balls[0].pocketed) {
        float nx = 0.0f, ny = 0.0f, power = 0.0f, pull_dist = 0.0f;
        int draw_cue = 0;

        if (turn == 2 && game_mode == 0 && cpu_thinking) {
            nx = cpu_nx;
            ny = cpu_ny;
            power = cpu_power;
            draw_cue = 1;
            
            if (cpu_timer < 1.0f) {
                pull_dist = (cpu_timer / 1.0f) * (power * 0.5f);
            } else if (cpu_timer < 1.3f) {
                pull_dist = power * 0.5f;
            } else if (cpu_timer < 1.4f) {
                float f = (cpu_timer - 1.3f) / 0.1f;
                pull_dist = (1.0f - f) * (power * 0.5f);
            }
        } else if (dragging) {
            float dx = drag_start_x - drag_current_x;
            float dy = drag_start_y - drag_current_y;
            power = sqrtf(dx*dx+dy*dy);
            
            if (power > 2.0f) {
                nx = dx / power;
                ny = dy / power;
                draw_cue = 1;
                pull_dist = power * 0.5f;
            }
        }

        if (draw_cue) {
            if (pull_dist > 40.0f) pull_dist = 40.0f;
            if (power > MAX_POWER) power = MAX_POWER;
            
            /* The CUE (brown stick) */
            Vector2 tip = { balls[0].x - nx * (BALL_SIZE/2 + 4 + pull_dist), 
                            balls[0].y - ny * (BALL_SIZE/2 + 4 + pull_dist) };
            Vector2 tail = { balls[0].x - nx * (BALL_SIZE/2 + 4 + pull_dist + 180), 
                             balls[0].y - ny * (BALL_SIZE/2 + 4 + pull_dist + 180) };
            
            DrawLineEx(tip, tail, 7.0f, (Color){100,60,30,255});
            DrawLineEx(tip, (Vector2){tip.x - nx*10, tip.y - ny*10}, 7.0f, (Color){220,200,180,255}); /* tip detail */
            
            /* The MIRA (aiming line) */
            float aim_len = power * 2.5f;
            if (aim_len > 300.0f) aim_len = 300.0f;

            /* Limit aiming guide to table bounds */
            if (nx > 0.001f) {
                float max_x = (TABLE_X + TABLE_W - balls[0].x) / nx;
                if (aim_len > max_x) aim_len = max_x;
            } else if (nx < -0.001f) {
                float max_x = (TABLE_X - balls[0].x) / nx;
                if (aim_len > max_x) aim_len = max_x;
            }
            if (ny > 0.001f) {
                float max_y = (TABLE_Y + TABLE_H - balls[0].y) / ny;
                if (aim_len > max_y) aim_len = max_y;
            } else if (ny < -0.001f) {
                float max_y = (TABLE_Y - balls[0].y) / ny;
                if (aim_len > max_y) aim_len = max_y;
            }

            if (aim_len > 10.0f) {
                /* Removed aiming line */
                int sx = (int)(balls[0].x + nx * aim_len);
                int sy = (int)(balls[0].y + ny * aim_len);
                DrawRectangle(sx - 3, sy - 3, 6, 6, (Color){255,255,255,120});
            }

            int pw_bar = (int)(power / MAX_POWER * 100);
            DrawRectangle(10, PH-25, 100, 10, (Color){40,40,40,255});
            DrawRectangle(10, PH-25, pw_bar, 10, (Color){255,(int)(255*(1-power/MAX_POWER)),50,255});
        }
    }

    /* HUD */
    DrawText(TextFormat(L("P1: %d", "P1: %d"), p1_score), 10, 10, 20, (Color){80,200,255,255});
    if (p1_type == 1) DrawText(L("(Lisas)", "(Solids)"), 80, 12, 16, WHITE);
    else if (p1_type == 2) DrawText(L("(Listradas)", "(Stripes)"), 80, 12, 16, WHITE);

    DrawText(TextFormat(L("P2: %d", "P2: %d"), p2_score), PW-100, 10, 20, (Color){255,100,130,255});
    if (p2_type == 1) DrawText(L("(Lisas)", "(Solids)"), PW-170, 12, 16, WHITE);
    else if (p2_type == 2) DrawText(L("(Listradas)", "(Stripes)"), PW-200, 12, 16, WHITE);

    const char *t_label = turn == 1 ? L("Vez: P1", "Turn: P1") : L("Vez: P2", "Turn: P2");
    Color t_color = turn == 1 ? (Color){80,200,255,255} : (Color){255,100,130,255};
    DrawText(t_label, (PW-MeasureText(t_label,18))/2, 10, 18, t_color);
    


    if (cballpool_state == 2) {
        DrawRectangle(0, 0, PW, PH, (Color){0,0,0,200});
        int center_y = PH / 2 - 80;
        
        const char *msg = L("FIM DE JOGO", "GAME OVER");
        if (winner == 1) msg = L("P1 VENCEU!", "P1 WINS!");
        else if (winner == 2) msg = (game_mode == 0) ? L("CPU VENCEU!", "CPU WINS!") : L("P2 VENCEU!", "P2 WINS!");
        
        int tw = MeasureText(msg, 50);
        DrawText(msg, (PW - tw) / 2, center_y, 50, GOLD);
        
        DrawText(TextFormat(L("Placar: P1:%d  P2:%d", "Score: P1:%d  P2:%d"), p1_score, p2_score), (PW - MeasureText(TextFormat(L("Placar: P1:%d  P2:%d", "Score: P1:%d  P2:%d"), p1_score, p2_score), 24)) / 2, center_y + 80, 24, WHITE);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        const char *hint = L("Pressione ENTER para Reiniciar", "Press ENTER to Restart");
        int hw = MeasureText(hint, 18);
        DrawText(hint, (PW - hw) / 2, center_y + 130, 18, (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)});
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
        DrawText(cd_text, (PW - scaled_w) / 2, (PH - scaled_size) / 2, scaled_size, cd_col);
    }


}

void cballpool_unload(void) { printf("[CBALLPOOL] Game unloaded\n"); }

int cballpool_get_mode(void) {
    return (cballpool_state == -1) ? 0 : game_mode;
}

Game CBALLPOOL_GAME = {
    .name = "C ball pool",
    .description = "Acerte as bolas coloridas na cacapa",
    .description_en = "Pot the colored balls in the pockets",
    .game_width = PW, .game_height = PH,
    .init = cballpool_init, .update = cballpool_update,
    .draw = cballpool_draw, .unload = cballpool_unload,
    .get_mode = cballpool_get_mode,
};
