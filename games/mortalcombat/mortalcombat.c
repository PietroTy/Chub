/**
 * @file mortalcombat.c
 * @brief Fighting game — rectangle-based, 2 players, Chub style.
 * P1: WASD move, U=punch, I=kick, O=block
 * P2: Arrows move, KP1=punch, KP2=kick, KP3=block
 */
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "raylib/raylib.h"
#include "hub.h"
#include "game.h"
#include "hub.h"
static int cd_num;
static float cd_timer;

#define FW  640
#define FH  480
#define FLOOR_Y 380
#define GRAVITY 900.0f
#define JUMP_SPEED 520.0f
#define MOVE_SPEED 180.0f

#define MAX_HP 100
#define ROUNDS_TO_WIN 5

/* ── Colors ── */
#define BG_SKY    (Color){10,8,20,255}
#define COL_FLOOR (Color){35,25,55,255}
#define COL_P1    (Color){80,200,255,255}
#define COL_P2    (Color){255,90,120,255}
#define COL_HIT   (Color){255,230,50,255}
#define COL_PROJ  (Color){255,200,0,255}
#define COL_HP1   (Color){50,220,100,255}
#define COL_HP2   (Color){220,50,80,255}
#define COL_WIN   (Color){255,220,80,255}
#define COL_DIM   (Color){0,0,0,160}

/* ── Player body dims ── */
#define BODY_W  32
#define BODY_H  64
#define HEAD_R  10
#define DUCK_H  36

/* ── Hitbox dims ── */
#define PUNCH_W 44
#define PUNCH_H 14
#define KICK_W  20
#define KICK_H  36

typedef enum {
    ST_IDLE, ST_WALK_F, ST_WALK_B,
    ST_JUMP, ST_CROUCH,
    ST_PUNCH, ST_KICK,
    ST_AIR_PUNCH, ST_AIR_KICK,
    ST_CROUCH_PUNCH, ST_CROUCH_KICK,
    ST_HADOUKEN,
    ST_BLOCK, ST_HIT, ST_WIN, ST_DEAD
} PState;

typedef struct {
    int dir; // 0=None, 1=U, 2=D, 3=L, 4=R, 5=DL, 6=DR, 7=UL, 8=UR
    float time;
} CmdInput;

#define CMD_BUFFER_SIZE 12

typedef struct {
    float x, y, vx, vy;
    int   hp, wins;
    bool  facing_right;
    PState state;
    float anim_t;   /* timer for current state */
    bool  on_ground;
    /* projectile */
    float px, py, pvx;
    bool  proj_active;
    float proj_t;
    /* input buffer for specials */
    CmdInput cmd_buffer[CMD_BUFFER_SIZE];
    int cmd_head;
    bool has_hit; /* ensures only 1 hit per attack animation */
} Fighter;

typedef enum {
    GAME_PLAYING,
    GAME_ROUND_END,
    GAME_MATCH_OVER
} GameState;

static Fighter p1, p2;
static GameState gstate;
static int game_mode;
static bool in_menu;
static float state_timer;
static int round_num;
static char announce_text[64];
static Sound pop_sound;
static bool sound_loaded = false;

/* ─────────────────────── helpers ─────────────────────── */


static void add_cmd_input(Fighter *f, int dir) {
    if (dir == 0) return;
    int last = f->cmd_head;
    if (f->cmd_buffer[last].dir == dir) return;
    f->cmd_head = (f->cmd_head + 1) % CMD_BUFFER_SIZE;
    f->cmd_buffer[f->cmd_head].dir = dir;
    f->cmd_buffer[f->cmd_head].time = (float)GetTime();
}

static bool check_qcf(Fighter *f) {
    float now = (float)GetTime();
    int fwd = f->facing_right ? 4 : 3;
    int dfwd = f->facing_right ? 6 : 5;
    
    bool found_fwd = false;
    for (int i = 0; i < CMD_BUFFER_SIZE; i++) {
        int idx = (f->cmd_head - i + CMD_BUFFER_SIZE) % CMD_BUFFER_SIZE;
        if (now - f->cmd_buffer[idx].time > 0.6f) break;
        int d = f->cmd_buffer[idx].dir;
        
        if (!found_fwd) {
            if (d == fwd || d == dfwd) found_fwd = true;
        } else {
            if (d == 2 || d == dfwd) return true;
        }
    }
    return false;
}

static void reset_round(void) {
    p1.x = 120; p1.y = FLOOR_Y - BODY_H; p1.vx = 0; p1.vy = 0;
    p1.hp = MAX_HP; p1.facing_right = true; p1.state = ST_IDLE;
    p1.anim_t = 0; p1.on_ground = true;
    p1.proj_active = false;
    p1.cmd_head = 0;
    for(int i=0; i<CMD_BUFFER_SIZE; i++) p1.cmd_buffer[i].dir = 0;

    p2.x = FW - 120 - BODY_W; p2.y = FLOOR_Y - BODY_H;
    p2.vx = 0; p2.vy = 0;
    p2.hp = MAX_HP; p2.facing_right = false; p2.state = ST_IDLE;
    p2.anim_t = 0; p2.on_ground = true;
    p2.proj_active = false;
    p2.cmd_head = 0;
    for(int i=0; i<CMD_BUFFER_SIZE; i++) p2.cmd_buffer[i].dir = 0;

    gstate = GAME_PLAYING;
    cd_num = 3;
    cd_timer = 0;
    snprintf(announce_text, sizeof(announce_text), "ROUND %d", round_num);
}

static Rectangle body_rect(Fighter *f) {
    bool is_crouching = (f->state == ST_CROUCH || f->state == ST_CROUCH_PUNCH || f->state == ST_CROUCH_KICK);
    float bh = is_crouching ? DUCK_H : BODY_H;
    float bw = is_crouching ? BODY_W + 12 : BODY_W;
    float bx = f->x - (bw - BODY_W)/2;
    float by = f->y + (BODY_H - (is_crouching ? DUCK_H : BODY_H));
    return (Rectangle){ bx, by, bw, bh };
}

static Rectangle head_rect(Fighter *f) {
    bool is_crouching = (f->state == ST_CROUCH || f->state == ST_CROUCH_PUNCH || f->state == ST_CROUCH_KICK);
    float by = f->y + (BODY_H - (is_crouching ? DUCK_H : BODY_H));
    return (Rectangle){ f->x + BODY_W/2 - HEAD_R, by - HEAD_R*2, HEAD_R*2, HEAD_R*2 };
}

static bool rects_overlap(Rectangle a, Rectangle b) {
    return a.x < b.x+b.width && a.x+a.width > b.x &&
           a.y < b.y+b.height && a.y+a.height > b.y;
}

/* returns hitbox for an attacking fighter */
static bool get_attack_rect(Fighter *f, Rectangle *out) {
    bool is_crouching = (f->state == ST_CROUCH || f->state == ST_CROUCH_PUNCH || f->state == ST_CROUCH_KICK);
    float bh = is_crouching ? DUCK_H : BODY_H;
    float by = f->y + (BODY_H - (is_crouching ? DUCK_H : BODY_H));

    if ((f->state == ST_PUNCH || f->state == ST_AIR_PUNCH || f->state == ST_CROUCH_PUNCH) && !f->has_hit) {
        float bw = is_crouching ? BODY_W + 12 : BODY_W;
        float bx = f->x - (bw - BODY_W)/2;
        float att_mult = (f->anim_t < 0.08f || f->anim_t > 0.22f) ? 0.5f : 1.0f;
        float cur_w = PUNCH_W * att_mult;
        float ox = f->facing_right ? (bx + bw - 4 - f->x) : (bx - cur_w + 4 - f->x);
        float oy = (f->state == ST_CROUCH_PUNCH) ? -10 : 4;
        *out = (Rectangle){ f->x + ox, by + oy, cur_w, PUNCH_H };
        return true;
    }
    if ((f->state == ST_KICK || f->state == ST_AIR_KICK) && !f->has_hit) {
        float bw = is_crouching ? BODY_W + 12 : BODY_W;
        float bx = f->x - (bw - BODY_W)/2;
        float att_mult = (f->anim_t < 0.08f || f->anim_t > 0.27f) ? 0.5f : 1.0f;
        float cur_w = KICK_W * 2.0f * att_mult;
        float ox = f->facing_right ? (bx + bw - 10 - f->x) : (bx - cur_w + 10 - f->x);
        float oy = 28; 
        *out = (Rectangle){ f->x + ox, by + oy, cur_w, PUNCH_H };
        return true;
    }
    if (f->state == ST_CROUCH_KICK && !f->has_hit) {
        float bw = is_crouching ? BODY_W + 12 : BODY_W;
        float bx = f->x - (bw - BODY_W)/2;
        float att_mult = (f->anim_t < 0.08f || f->anim_t > 0.27f) ? 0.5f : 1.0f;
        float cur_w = KICK_W * 2.6f * att_mult;
        float ox = f->facing_right ? (bx + bw - 10 - f->x) : (bx - cur_w + 10 - f->x);
        float oy = (bh * 0.55f) - 4;
        *out = (Rectangle){ f->x + ox, by + oy, cur_w, 12 };
        return true;
    }
    return false;
}

static void do_attack(Fighter *attacker, Fighter *defender, int dmg) {
    if (sound_loaded) PlaySound(pop_sound);
    if (defender->state == ST_BLOCK) dmg /= 4;
    if (defender->state == ST_HIT || defender->state == ST_DEAD) return;
    defender->hp -= dmg;
    if (defender->hp < 0) defender->hp = 0;
    defender->state = ST_HIT;
    defender->anim_t = 0;
    /* knockback */
    defender->vx = attacker->facing_right ? 180.0f : -180.0f;
}

/* ─────────────────────── update fighter ─────────────────────── */

static void update_fighter(Fighter *f, Fighter *opp, float dt,
                            bool up, bool dn, bool lt, bool rt,
                            bool punch, bool kick)
{
    if (f->state == ST_WIN || f->state == ST_DEAD) return;

    f->anim_t += dt;

    /* face opponent */
    f->facing_right = (opp->x + BODY_W/2) > (f->x + BODY_W/2);

    /* Command buffer recording */
    int cur_dir = 0;
    if (up) cur_dir = 1; else if (dn) cur_dir = 2;
    if (lt) {
        if (cur_dir == 2) cur_dir = 5; 
        else if (cur_dir == 1) cur_dir = 7; 
        else cur_dir = 3; 
    } else if (rt) {
        if (cur_dir == 2) cur_dir = 6; 
        else if (cur_dir == 1) cur_dir = 8; 
        else cur_dir = 4; 
    }
    add_cmd_input(f, cur_dir);

    /* resolve timed states */
    if (f->state == ST_HIT  && f->anim_t > 0.30f) { f->state = ST_IDLE; f->vx = 0; }
    if ((f->state == ST_PUNCH || f->state == ST_AIR_PUNCH || f->state == ST_CROUCH_PUNCH) && f->anim_t > 0.35f) f->state = f->on_ground ? ST_IDLE : ST_JUMP;
    if ((f->state == ST_KICK  || f->state == ST_AIR_KICK || f->state == ST_CROUCH_KICK)  && f->anim_t > 0.45f) f->state = f->on_ground ? ST_IDLE : ST_JUMP;
    if (f->state == ST_HADOUKEN && f->anim_t > 0.5f) f->state = ST_IDLE;

    bool busy = (f->state == ST_HIT || f->state == ST_PUNCH || f->state == ST_KICK || 
                 f->state == ST_AIR_PUNCH || f->state == ST_AIR_KICK ||
                 f->state == ST_CROUCH_PUNCH || f->state == ST_CROUCH_KICK ||
                 f->state == ST_HADOUKEN || f->state == ST_WIN || f->state == ST_DEAD);

    if (!busy) {
        /* move */
        if (rt) { f->vx = MOVE_SPEED;  f->state = f->on_ground ? ST_WALK_F : f->state; }
        else if (lt) { f->vx = -MOVE_SPEED; f->state = f->on_ground ? ST_WALK_B : f->state; }
        else if (f->on_ground && !dn) {
            f->vx = 0;
            f->state = ST_IDLE;
        }

        /* block (hold back) - Allow moving while blocking */
        bool is_holding_back = (f->facing_right && lt) || (!f->facing_right && rt);
        if (is_holding_back && f->on_ground && !up && !dn) {
            f->state = ST_BLOCK;
            // Velocity is already set by move logic
        }
        
        /* jump */
        if (up && f->on_ground && f->state != ST_CROUCH) {
            f->vy = -JUMP_SPEED;
            f->on_ground = false;
            f->state = ST_JUMP;
        }
        /* crouch */
        else if (dn && f->on_ground) {
            f->state = ST_CROUCH;
            f->vx = 0;
        }

        /* attacks */
        if (punch) {
            if (!f->on_ground) { f->state = ST_AIR_PUNCH; f->anim_t = 0; f->has_hit = false; }
            else if (dn)       { f->state = ST_CROUCH_PUNCH; f->anim_t = 0; f->vx = 0; f->has_hit = false; }
            else if (check_qcf(f) && !f->proj_active) {
                /* Hadouken! */
                f->state = ST_HADOUKEN; f->anim_t = 0; f->vx = 0; f->has_hit = false;
                f->proj_active = true;
                f->px = f->x + (f->facing_right ? BODY_W : 0);
                f->py = f->y + 20;
                f->pvx = f->facing_right ? 380.0f : -380.0f;
                f->proj_t = 0;
            }
            else               { f->state = ST_PUNCH; f->anim_t = 0; f->vx = 0; f->has_hit = false; }
        }
        if (kick) {
            if (!f->on_ground) { f->state = ST_AIR_KICK; f->anim_t = 0; f->has_hit = false; }
            else if (dn)       { f->state = ST_CROUCH_KICK; f->anim_t = 0; f->vx = 0; f->has_hit = false; }
            else               { f->state = ST_KICK;  f->anim_t = 0; f->vx = 0; f->has_hit = false; }
        }
    }

    /* physics */
    f->x += f->vx * dt;
    f->y += f->vy * dt;
    f->vy += GRAVITY * dt;

    /* floor */
    float floor_y = FLOOR_Y - BODY_H;
    if (f->y >= floor_y) { f->y = floor_y; f->vy = 0; f->on_ground = true; if (f->state == ST_JUMP) f->state = ST_IDLE; }

    /* walls */
    if (f->x < 0)            f->x = 0;
    if (f->x > FW - BODY_W)  f->x = FW - BODY_W;

    /* projectile update */
    if (f->proj_active) {
        f->proj_t += dt;
        f->px += f->pvx * dt;
        if (f->px < -20 || f->px > FW + 20 || f->proj_t > 3.0f) f->proj_active = false;
        /* proj hits opponent */
        Rectangle pr = { f->px - 12, f->py - 6, 24, 12 };
        if (rects_overlap(pr, body_rect(opp)) || rects_overlap(pr, head_rect(opp))) {
            do_attack(f, opp, 12);
            f->proj_active = false;
        }
    }
}

/* ─────────────────────── collision between players ─────────────────────── */

static void check_attacks(void) {
    Rectangle h;
    if (get_attack_rect(&p1, &h) && !p1.has_hit) {
        if (rects_overlap(h, body_rect(&p2)) || rects_overlap(h, head_rect(&p2))) {
            int dmg = (p1.state == ST_PUNCH || p1.state == ST_AIR_PUNCH || p1.state == ST_CROUCH_PUNCH) ? 10 : 15;
            do_attack(&p1, &p2, dmg);
            p1.has_hit = true;
        }
    }
    if (get_attack_rect(&p2, &h) && !p2.has_hit) {
        if (rects_overlap(h, body_rect(&p1)) || rects_overlap(h, head_rect(&p1))) {
            int dmg = (p2.state == ST_PUNCH || p2.state == ST_AIR_PUNCH || p2.state == ST_CROUCH_PUNCH) ? 10 : 15;
            do_attack(&p2, &p1, dmg);
            p2.has_hit = true;
        }
    }
}

static void push_apart(void) {
    Rectangle r1 = body_rect(&p1), r2 = body_rect(&p2);
    if (!rects_overlap(r1, r2)) return;
    float cx1 = r1.x + r1.width/2, cx2 = r2.x + r2.width/2;
    float overlap = (r1.width/2 + r2.width/2) - fabsf(cx2-cx1);
    if (overlap > 0) {
        float push = overlap / 2.0f + 1.0f;
        if (cx1 < cx2) { p1.x -= push; p2.x += push; }
        else           { p1.x += push; p2.x -= push; }
    }
}

/* ─────────────────────── draw ─────────────────────── */

static void draw_fighter(Fighter *f, Color body_col) {
    bool is_p1 = (body_col.b > body_col.r);
    bool hit_flash = (f->state == ST_HIT && (int)(f->anim_t * 12) % 2 == 0);
    Color c = hit_flash ? COL_HIT : body_col;

    /* manual breathing/animation */
    float breathe_h = 0;
    float breathe_w = 0;
    bool should_breathe = (f->state == ST_IDLE || f->state == ST_BLOCK || f->state == ST_CROUCH || f->state == ST_JUMP);
    if (should_breathe) {
        if ((int)(state_timer * 2.5f) % 2 == 0) {
            breathe_h = -2;
            breathe_w = 2;
        }
    }

    /* body height/width depend on crouch */
    bool is_crouching = (f->state == ST_CROUCH || f->state == ST_CROUCH_PUNCH || f->state == ST_CROUCH_KICK);
    float bh = (is_crouching ? DUCK_H : BODY_H) + breathe_h;
    float bw = (is_crouching ? BODY_W + 12 : BODY_W) + breathe_w;
    float bx = f->x - (bw - BODY_W)/2;
    float by = f->y + (BODY_H - (is_crouching ? DUCK_H : BODY_H)) - breathe_h;

    /* legs animation (manual steps) */
    bool is_kicking = (f->state == ST_KICK || f->state == ST_AIR_KICK || f->state == ST_CROUCH_KICK);
    int step = (int)(f->anim_t * 10.0f) % 4;
    bool walking = (f->state == ST_WALK_F || f->state == ST_WALK_B || (f->state == ST_BLOCK && f->vx != 0));
    
    int l1h = (int)(bh * 0.45f);
    int l2h = (int)(bh * 0.45f);
    if (walking) {
        if (step == 1) { l1h -= 6; l2h += 4; }
        else if (step == 3) { l1h += 4; l2h -= 6; }
    }

    // Facing right: L1 (left) is back, L2 (right) is front
    // Facing left: L2 (right) is back, L1 (left) is front
    bool hide_l1 = (is_kicking && f->facing_right);
    bool hide_l2 = (is_kicking && !f->facing_right);

    int torso_h = (int)(bh * 0.55f);
    int leg_y_top = (int)by + torso_h - 1; // 1px overlap to fix bleeding

    /* leg 1 (left) */
    if (!hide_l1) {
        DrawRectangle((int)f->x + 2, leg_y_top, BODY_W/2 - 3, l1h, c);
        DrawRectangleLinesEx((Rectangle){f->x+2, (float)leg_y_top, BODY_W/2-3, (float)l1h}, 2, (Color){0,0,0,100});
    }
    /* leg 2 (right) */
    if (!hide_l2) {
        DrawRectangle((int)f->x + BODY_W/2 + 1, leg_y_top, BODY_W/2 - 3, l2h, c);
        DrawRectangleLinesEx((Rectangle){f->x+BODY_W/2+1, (float)leg_y_top, BODY_W/2-3, (float)l2h}, 2, (Color){0,0,0,100});
    }

    /* torso */
    DrawRectangle((int)bx, (int)by, (int)bw, torso_h, c);
    DrawRectangleLinesEx((Rectangle){bx, by, bw, (float)torso_h}, 3, (Color){0,0,0,100});

    /* head */
    int hx = (int)(f->x + BODY_W/2);
    int hy = (int)by - HEAD_R + 1; // 1px overlap
    int hs = HEAD_R * 2;
    DrawRectangle(hx - HEAD_R, hy - HEAD_R, hs, hs, c);
    DrawRectangleLinesEx((Rectangle){(float)hx-HEAD_R, (float)hy-HEAD_R, (float)hs, (float)hs}, 3, (Color){0,0,0,100});

    /* Hat */
    if (is_p1) {
        // Fez
        Rectangle fez_r = {hx - 9, hy - HEAD_R - 12, 18, 12};
        DrawRectangleRec(fez_r, (Color){180, 20, 40, 255});
        DrawRectangleLinesEx(fez_r, 2, (Color){0,0,0,100});
        // Minimalist yellow tassel
        DrawRectangle(hx + 5, hy - HEAD_R - 8, 4, 4, YELLOW);
        DrawRectangleLinesEx((Rectangle){hx + 5, hy - HEAD_R - 8, 4, 4}, 1, BLACK);
    } else {
        // Top Hat
        Rectangle brim = {hx - 15, hy - HEAD_R - 2, 30, 4};
        Rectangle top = {hx - 10, hy - HEAD_R - 18, 20, 16};
        Rectangle band = {hx - 10, hy - HEAD_R - 6, 20, 4};
        DrawRectangleRec(brim, BLACK);
        DrawRectangleRec(top, BLACK);
        DrawRectangleRec(band, (Color){60, 20, 20, 255});
        DrawRectangleLinesEx(brim, 2, (Color){10,10,10,100});
        DrawRectangleLinesEx(top, 2, (Color){10,10,10,100});
    }

    /* eyes (Two Eyes) */
    Color eye_col = WHITE;
    // Left eye
    DrawRectangle(hx - 6, hy - 5, 4, 4, eye_col);
    DrawRectangle(hx - 6 + (f->facing_right ? 2 : 0), hy - 4, 2, 2, BLACK);
    // Right eye
    DrawRectangle(hx + 2, hy - 5, 4, 4, eye_col);
    DrawRectangle(hx + 2 + (f->facing_right ? 2 : 0), hy - 4, 2, 2, BLACK);

    /* attacks */
    if ((f->state == ST_PUNCH || f->state == ST_AIR_PUNCH || f->state == ST_CROUCH_PUNCH) && f->anim_t < 0.30f) {
        float att_mult = (f->anim_t < 0.08f || f->anim_t > 0.22f) ? 0.5f : 1.0f;
        int arm_w = (int)(PUNCH_W * att_mult);
        // Use actual torso edge for connection
        int arm_x = f->facing_right ? (int)(bx + bw - 4) : (int)(bx - arm_w + 4);
        float arm_y = by + 4; 
        if (f->state == ST_CROUCH_PUNCH) arm_y = by - 10;
        DrawRectangle(arm_x, (int)arm_y, arm_w, PUNCH_H, c);
        DrawRectangleLinesEx((Rectangle){(float)arm_x, arm_y, (float)arm_w, (float)PUNCH_H}, 2, (Color){0,0,0,100});
    }

    if ((f->state == ST_KICK || f->state == ST_AIR_KICK || f->state == ST_CROUCH_KICK) && f->anim_t < 0.35f) {
        float att_mult = (f->anim_t < 0.08f || f->anim_t > 0.27f) ? 0.5f : 1.0f;
        int base_w = (f->state == ST_CROUCH_KICK) ? (int)(KICK_W * 2.6f) : (int)(KICK_W * 2.0f);
        int leg_w = (int)(base_w * att_mult);
        float leg_oy = 28; // Waist height
        int leg_h = PUNCH_H;
        if (f->state == ST_CROUCH_KICK) {
            leg_oy = (bh * 0.55f) - 4; // Connect to torso bottom
            leg_h = 12;
        }
        // Use actual torso edge for connection with more overlap
        int leg_x = f->facing_right ? (int)(bx + bw - 10) : (int)(bx - leg_w + 10);
        float leg_y = by + leg_oy;
        DrawRectangle(leg_x, (int)leg_y, leg_w, leg_h, c);
        DrawRectangleLinesEx((Rectangle){(float)leg_x, leg_y, (float)leg_w, (float)leg_h}, 2, (Color){0,0,0,100});
    }

    /* Hadouken animation (Two Hands) */
    if (f->state == ST_HADOUKEN && f->anim_t < 0.4f) {
        int h_w = 32;
        int h_x = f->facing_right ? (int)(f->x + BODY_W) : (int)(f->x - h_w);
        /* top hand */
        DrawRectangle(h_x, (int)(by + 10), h_w, 12, c);
        DrawRectangleLinesEx((Rectangle){(float)h_x, by+10, (float)h_w, 12}, 1, (Color){0,0,0,80});
        /* bottom hand */
        DrawRectangle(h_x, (int)(by + 24), h_w, 12, c);
        DrawRectangleLinesEx((Rectangle){(float)h_x, by+24, (float)h_w, 12}, 1, (Color){0,0,0,80});
    }

    /* block shield */
    if (f->state == ST_BLOCK) {
        Color sc = c; sc.a = 180;
        int sx = f->facing_right ? (int)f->x + BODY_W - 4 : (int)f->x - 14;
        DrawRectangle(sx, (int)by - 4, 14, (int)(bh * 0.7f), sc);
        DrawRectangleLinesEx((Rectangle){(float)sx, (float)by-4, 14, bh*0.7f}, 2, WHITE);
    }

    /* projectile */
    if (f->proj_active) {
        float pulse = 1.0f + 0.2f * sinf(f->proj_t * 20.0f);
        int pr = (int)(10 * pulse);
        int p_out = (pr + 4) * 2;
        DrawRectangle((int)f->px - (pr + 4), (int)f->py - (pr + 4), p_out, p_out, (Color){255,200,0,60});
        int p_mid = pr * 2;
        DrawRectangle((int)f->px - pr, (int)f->py - pr, p_mid, p_mid, COL_PROJ);
        int p_in = (pr - 4) * 2;
        if (p_in > 0) DrawRectangle((int)f->px - (pr - 4), (int)f->py - (pr - 4), p_in, p_in, WHITE);
    }
}

static void draw_hud(void) {
    int bar_w = 220, bar_h = 20;
    int p1_hp_w = (int)(bar_w * p1.hp / (float)MAX_HP);
    int p2_hp_w = (int)(bar_w * p2.hp / (float)MAX_HP);

    /* P1 bar (left) */
    DrawRectangle(20, 20, bar_w, bar_h, (Color){40,20,30,255});
    DrawRectangle(20, 20, p1_hp_w, bar_h, COL_HP1);
    DrawRectangleLinesEx((Rectangle){20,20,bar_w,bar_h}, 2, COL_P1);
    DrawText(L("P1", "P1"), 20, 44, 14, COL_P1);

    /* P2 bar (right) */
    DrawRectangle(FW - 20 - bar_w, 20, bar_w, bar_h, (Color){40,20,30,255});
    DrawRectangle(FW - 20 - bar_w + (bar_w - p2_hp_w), 20, p2_hp_w, bar_h, COL_HP2);
    DrawRectangleLinesEx((Rectangle){FW-20-bar_w,20,bar_w,bar_h}, 2, COL_P2);
    const char *p2_name = (game_mode == 1) ? L("P2", "P2") : L("CPU", "CPU");
    int tw = MeasureText(p2_name, 14);
    DrawText(p2_name, FW - 20 - tw, 44, 14, COL_P2);

    /* Wins dots */
    for (int i = 0; i < ROUNDS_TO_WIN; i++) {
        Color dc = i < p1.wins ? COL_P1 : (Color){60,60,80,255};
        DrawRectangle(20 + i*16, 62, 10, 10, dc);
        DrawRectangleLinesEx((Rectangle){20.0f + i*16, 62, 10, 10}, 1, BLACK);
        dc = i < p2.wins ? COL_P2 : (Color){60,60,80,255};
        DrawRectangle(FW - 20 - i*16 - 10, 62, 10, 10, dc);
        DrawRectangleLinesEx((Rectangle){FW - 20.0f - i*16 - 10, 62, 10, 10}, 1, BLACK);
    }

    /* Round label */
    char rlabel[32]; snprintf(rlabel, sizeof(rlabel), L("Round %d", "Round %d"), round_num);
    int rw = MeasureText(rlabel, 16);
    DrawText(rlabel, (FW-rw)/2, 16, 16, (Color){180,160,200,255});
}

static void draw_scene(void) {
    /* sky gradient via 3 rects */
    DrawRectangle(0, 0, FW, FH/3, (Color){10,5,25,255});
    DrawRectangle(0, FH/3, FW, FH/3, (Color){18,10,40,255});
    DrawRectangle(0, 2*FH/3, FW, FH/3, (Color){25,15,50,255});

    /* stars */
    srand(42);
    for (int i = 0; i < 60; i++) {
        int sx = rand() % FW, sy = rand() % (FLOOR_Y - 40);
        unsigned char a = (unsigned char)(100 + rand()%155);
        DrawPixel(sx, sy, (Color){255,255,255,a});
    }
    srand((unsigned int)GetTime()); /* restore randomness */

    /* floor */
    DrawRectangle(0, FLOOR_Y, FW, FH - FLOOR_Y, COL_FLOOR);
    DrawRectangle(0, FLOOR_Y, FW, 4, (Color){80,50,120,255});

    /* floor grid lines */
    for (int gx = 0; gx < FW; gx += 60) {
        DrawLine(gx, FLOOR_Y, gx + 40, FH, (Color){50,35,80,60});
    }
}

/* ─────────────────────── lifecycle ─────────────────────── */

void mortalcombat_init(void) {
    p1.wins = 0; p2.wins = 0;
    round_num = 1;
    reset_round();
    in_menu = true;
    cd_num = 0;
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;
    printf("[MORTALCOMBAT] Game initialized\n");
}

void mortalcombat_update(void) {
    if (cd_num > 0) {
        cd_timer += GetFrameTime();
        cd_num = 3 - (int)cd_timer;
        if (cd_num > 0) return;
    }
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    state_timer += dt;


    if (in_menu) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            game_mode = 0;
            in_menu = false;
            cd_num = 3;
            cd_timer = 0;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            game_mode = 1;
            in_menu = false;
            cd_num = 3;
            cd_timer = 0;
        }
        return;
    }

    if (gstate == GAME_ROUND_END || gstate == GAME_MATCH_OVER) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (gstate == GAME_MATCH_OVER) {
                p1.wins = 0; p2.wins = 0; round_num = 1;
            } else {
                round_num++;
            }
            reset_round();
        }
        return;
    }

    /* ── Inputs ── */
    bool p1_up    = IsKeyDown(KEY_W);
    bool p1_dn    = IsKeyDown(KEY_S);
    bool p1_lt    = IsKeyDown(KEY_A);
    bool p1_rt    = IsKeyDown(KEY_D);
    bool p1_punch = IsKeyPressed(KEY_U);
    bool p1_kick  = IsKeyPressed(KEY_I);

    if (game_mode == 0) {
        if (IsKeyDown(KEY_UP)) p1_up = true;
        if (IsKeyDown(KEY_DOWN)) p1_dn = true;
        if (IsKeyDown(KEY_LEFT)) p1_lt = true;
        if (IsKeyDown(KEY_RIGHT)) p1_rt = true;
        if (IsKeyPressed(KEY_KP_1)) p1_punch = true;
        if (IsKeyPressed(KEY_KP_2)) p1_kick = true;
    }

    bool p2_up, p2_dn, p2_lt, p2_rt, p2_punch, p2_kick;
    if (game_mode == 1) {
        p2_up    = IsKeyDown(KEY_UP);
        p2_dn    = IsKeyDown(KEY_DOWN);
        p2_lt    = IsKeyDown(KEY_LEFT);
        p2_rt    = IsKeyDown(KEY_RIGHT);
        p2_punch = IsKeyPressed(KEY_KP_1);
        p2_kick  = IsKeyPressed(KEY_KP_2);
    } else {
        p2_up = p2_dn = p2_lt = p2_rt = p2_punch = p2_kick = false;
        float dist = p1.x - p2.x; // positive if p1 is to the right
        
        bool p1_attacking = (p1.state == ST_PUNCH || p1.state == ST_KICK || 
                             p1.state == ST_AIR_PUNCH || p1.state == ST_AIR_KICK || 
                             p1.state == ST_CROUCH_PUNCH || p1.state == ST_CROUCH_KICK ||
                             p1.proj_active);

        if (p1_attacking && fabsf(dist) < 180.0f) {
            if (GetRandomValue(0, 100) < 50) {
                // Block by moving away
                if (p2.facing_right) p2_lt = true;
                else p2_rt = true;
            }
        }
        
        if (gstate == GAME_PLAYING) {
            if (fabsf(dist) > 70.0f) {
                if (dist > 0) p2_rt = true;
                else p2_lt = true;
                // Random projectile chance
                if (GetRandomValue(0, 100) < 2) {
                    p2_dn = true; p2_rt = !p2.facing_right; p2_lt = p2.facing_right; // fake QCF for logic? 
                    // Actually CPU can just "cheat" or we can simulate inputs. 
                    // Let's just give it a small chance to punch which might trigger Hadouken if it happens to have the buffer.
                    // For now, CPU won't use Hadouken unless we implement full input simulation.
                    p2_punch = true; 
                }
            } else {
                int r = GetRandomValue(0, 100);
                if (r < 8) p2_punch = true;
                else if (r < 15) p2_kick = true;
                else if (r < 20) p2_dn = true;
            }
        }
    }

    update_fighter(&p1, &p2, dt, p1_up, p1_dn, p1_lt, p1_rt, p1_punch, p1_kick);
    update_fighter(&p2, &p1, dt, p2_up, p2_dn, p2_lt, p2_rt, p2_punch, p2_kick);
    push_apart();
    check_attacks();

    /* ── Check KO ── */
    if (p1.hp <= 0 || p2.hp <= 0) {
        const char *p2_name = (game_mode == 1) ? "P2" : "CPU";
        if (p1.hp <= 0) { p2.wins++; p1.state = ST_DEAD; snprintf(announce_text, sizeof(announce_text), L("%s VENCEU O ROUND!", "%s WINS THE ROUND!"), p2_name); }
        else            { p1.wins++; p2.state = ST_DEAD; snprintf(announce_text, sizeof(announce_text), L("P1 VENCEU O ROUND!", "P1 WINS THE ROUND!")); }

        if (p1.wins >= ROUNDS_TO_WIN || p2.wins >= ROUNDS_TO_WIN) {
            snprintf(announce_text, sizeof(announce_text), "%s", p1.wins >= ROUNDS_TO_WIN ? L("P1 VENCEU A LUTA!", "P1 WINS THE MATCH!") : TextFormat(L("%s VENCEU A LUTA!", "%s WINS THE MATCH!"), p2_name));
            gstate = GAME_MATCH_OVER;
        } else {
            gstate = GAME_ROUND_END;
        }
        state_timer = 0;
    }
}

void mortalcombat_draw(void) {

    if (in_menu) {
        DrawRectangle(0, 0, FW, FH, BLACK);
        DrawText(L("SELECIONE O MODO", "SELECT MODE"), FW / 2 - MeasureText(L("SELECIONE O MODO", "SELECT MODE"), 40) / 2, 100, 40, WHITE);
        DrawText(L("<- 1 Jogador (Vs CPU)", "<- 1 Player (Vs CPU)"), FW / 2 - 250, 250, 20, WHITE);
        DrawText(L("2 Jogadores ->", "2 Players ->"), FW / 2 + 50, 250, 20, WHITE);
        DrawText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), FW / 2 - MeasureText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), 20) / 2, 400, 20, DARKGRAY);
        return;
    }
    draw_scene();
    draw_fighter(&p1, COL_P1);
    draw_fighter(&p2, COL_P2);
    draw_hud();

    if (gstate == GAME_ROUND_END || gstate == GAME_MATCH_OVER) {
        DrawRectangle(0, 0, FW, FH, (Color){0, 0, 0, 200});
        int sz = 46;
        int tw2 = MeasureText(announce_text, sz);
        DrawText(announce_text, (FW-tw2)/2, FH/2 - 50, sz, COL_WIN);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        const char *htext = gstate == GAME_MATCH_OVER
            ? L("Pressione ENTER para Nova Luta", "Press ENTER for New Match")
            : L("Pressione ENTER para Proximo Round", "Press ENTER for Next Round");
        int hw = MeasureText(htext, 18);
        DrawText(htext, (FW - hw)/2, FH/2+30, 18, (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)});
    }


    if (cd_num > 0) {
        DrawRectangle(0, 0, FW, FH, (Color){0, 0, 0, 180});
        
        char cd_text[16];
        snprintf(cd_text, sizeof(cd_text), "%d", cd_num);
        int cd_size = 100;
        float frac = cd_timer - (int)cd_timer;
        unsigned char alpha = (unsigned char)(255 * (1.0f - frac));
        float scale = 1.0f + frac * 0.5f;
        int scaled_size = (int)(cd_size * scale);
        int scaled_w = MeasureText(cd_text, scaled_size);
        Color cd_col = (Color){ 255, 255, 200, alpha };
        
        int fixed_center_y = FH / 2;
        int cd_y = fixed_center_y - scaled_size / 2 + 20;
        DrawText(cd_text, (FW - scaled_w) / 2, cd_y, scaled_size, cd_col);
        
        /* Draw ROUND text above countdown statically */
        int r_w = MeasureText(announce_text, 40);
        DrawText(announce_text, (FW - r_w) / 2, fixed_center_y - 80, 40, COL_WIN);
    }
}

void mortalcombat_unload(void) {
    if (sound_loaded) UnloadSound(pop_sound);
    printf("[MORTALCOMBAT] Game unloaded\n");
}

Game MORTALCOMBAT_GAME = {
    .name        = "mortal Combat",
    .description = "Combate intenso em arena pixel-art",
    .description_en = "Intense combat in a pixel-art arena",
    .game_width  = FW,
    .game_height = FH,
    .init        = mortalcombat_init,
    .update      = mortalcombat_update,
    .draw        = mortalcombat_draw,
    .unload      = mortalcombat_unload,
};
