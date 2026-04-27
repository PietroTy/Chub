/**
 * @file ponc.c
 * @brief Pong game module implementation.
 *
 * Classic Pong with:
 *   - Player 1 (left paddle, W/S or Arrow keys)
 *   - CPU opponent (right paddle, simple AI)
 *   - Ball with physics, angle variation on paddle hits
 *   - Score tracking (first to 5 wins)
 *   - Neon visual style (dark bg, glowing elements)
 *   - Victory screen with restart option
 *
 * Internal resolution: 640x480
 * All drawing happens at internal resolution (hub scales it).
 */
#include <math.h>
#include <stdio.h>

#include "raylib/raylib.h"
#include "game.h"
#include "hub.h"

// ══════════════════════════════════════════════
// CONSTANTS
// ══════════════════════════════════════════════

#define PONC_WIDTH   640
#define PONC_HEIGHT  480

#define PADDLE_WIDTH   12
#define PADDLE_HEIGHT  80
#define PADDLE_SPEED   300.0f
#define PADDLE_MARGIN  30

#define BALL_SIZE      10
#define BALL_SPEED_INIT 280.0f
#define BALL_SPEED_MAX  500.0f
#define BALL_SPEED_INC  15.0f

#define CPU_SPEED       250.0f
#define CPU_REACTION     0.85f   // How well CPU tracks the ball (0..1)

#define WIN_SCORE        5

// ── Colors (neon style) ──
#define PONC_BG          (Color){ 8, 8, 18, 255 }
#define PONC_COURT       (Color){ 20, 22, 35, 255 }
#define PONC_LINE        (Color){ 40, 45, 65, 255 }
#define PONC_PADDLE_P1   (Color){ 80, 200, 255, 255 }    // Cyan
#define PONC_PADDLE_CPU  (Color){ 255, 100, 130, 255 }    // Pink
#define PONC_BALL        (Color){ 255, 255, 200, 255 }    // Warm white
#define PONC_BALL_GLOW   (Color){ 255, 255, 150, 40 }
#define PONC_SCORE_COL   (Color){ 180, 190, 210, 255 }
#define PONC_TEXT_DIM     (Color){ 80, 90, 110, 255 }
#define PONC_WIN_TEXT     (Color){ 255, 220, 100, 255 }

// ══════════════════════════════════════════════
// GAME STATE
// ══════════════════════════════════════════════

typedef enum {
    PONC_STATE_MENU,
    PONC_STATE_COUNTDOWN,   // 3..2..1 before start/restart
    PONC_STATE_PLAYING,     // Active gameplay
    PONC_STATE_GOAL,        // Brief pause after scoring
    PONC_STATE_WIN,         // Someone won (first to WIN_SCORE)
} PongState;

typedef struct {
    float x, y;
    float w, h;
} Paddle;

typedef struct {
    float x, y;
    float vx, vy;
    float speed;
} Ball;

static PongState ponc_state;
static int game_mode;
static Paddle player;
static Paddle cpu;
static Ball ball;
static int score_p1;
static int score_cpu;
static float state_timer;
static int countdown_num;
static Sound pop_sound;
static bool sound_loaded = false;
static float ball_trail_alpha;

// ══════════════════════════════════════════════
// HELPERS
// ══════════════════════════════════════════════

static void reset_ball(int direction) {
    ball.x = PONC_WIDTH / 2.0f;
    ball.y = PONC_HEIGHT / 2.0f;
    ball.speed = BALL_SPEED_INIT;

    // Random angle between -30 and +30 degrees
    float angle = (float)(GetRandomValue(-30, 30)) * DEG2RAD;
    ball.vx = cosf(angle) * direction;
    ball.vy = sinf(angle);
}

static void start_countdown(void) {
    ponc_state = PONC_STATE_COUNTDOWN;
    state_timer = 0.0f;
    countdown_num = 3;
}

// ══════════════════════════════════════════════
// LIFECYCLE
// ══════════════════════════════════════════════

void ponc_init(void) {
    // Player paddle (left)
    player.x = PADDLE_MARGIN;
    player.y = PONC_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
    player.w = PADDLE_WIDTH;
    player.h = PADDLE_HEIGHT;

    // CPU paddle (right)
    cpu.x = PONC_WIDTH - PADDLE_MARGIN - PADDLE_WIDTH;
    cpu.y = PONC_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
    cpu.w = PADDLE_WIDTH;
    cpu.h = PADDLE_HEIGHT;

    // Scores
    score_p1 = 0;
    score_cpu = 0;

    // Ball
    reset_ball(1);

    // Start countdown
    ball_trail_alpha = 0.0f;
    ponc_state = PONC_STATE_MENU;
    countdown_num = 0; // stop the auto countdown

    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;
    printf("[PONC] Game initialized\n");
}

void ponc_update(void) {
    float dt = GetFrameTime();
    state_timer += dt;

    switch (ponc_state) {

                case PONC_STATE_MENU: {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                game_mode = 0;
                start_countdown();
                countdown_num = 3;
                state_timer = 0.0f;
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                game_mode = 1;
                start_countdown();
                countdown_num = 3;
                state_timer = 0.0f;
            }
            break;
        }

        case PONC_STATE_COUNTDOWN: {
            countdown_num = 3 - (int)state_timer;
            if (countdown_num <= 0) {
                ponc_state = PONC_STATE_PLAYING;
                state_timer = 0.0f;
            }
            // Allow paddle movement during countdown
            break;
        }

        case PONC_STATE_GOAL: {
            if (state_timer > 1.0f) {
                // Check for win
                if (score_p1 >= WIN_SCORE || score_cpu >= WIN_SCORE) {
                    ponc_state = PONC_STATE_WIN;
                    state_timer = 0.0f;
                } else {
                    // Reset ball and countdown
                    int dir = (score_p1 > score_cpu) ? -1 : 1;
                    reset_ball(dir);
                    start_countdown();
                }
            }
            break;
        }

        case PONC_STATE_WIN: {
            // Press ENTER to restart
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                score_p1 = 0;
                score_cpu = 0;
                reset_ball(1);
                start_countdown();
            }
            break;
        }

        case PONC_STATE_PLAYING: {
            // ── Player input ──
            int p1_up = IsKeyDown(KEY_W);
            int p1_down = IsKeyDown(KEY_S);
            
            if (game_mode == 0) {
                if (IsKeyDown(KEY_UP)) p1_up = 1;
                if (IsKeyDown(KEY_DOWN)) p1_down = 1;
            }

            if (p1_up) {
                player.y -= PADDLE_SPEED * dt;
            }
            if (p1_down) {
                player.y += PADDLE_SPEED * dt;
            }

            // Clamp player
            if (player.y < 0) player.y = 0;
            if (player.y + player.h > PONC_HEIGHT) player.y = PONC_HEIGHT - player.h;

            // ── Player 2 or CPU ──
            if (game_mode == 1) {
                if (IsKeyDown(KEY_UP)) cpu.y -= PADDLE_SPEED * dt;
                if (IsKeyDown(KEY_DOWN)) cpu.y += PADDLE_SPEED * dt;
            } else {
                float cpu_target = ball.y - cpu.h / 2.0f;
            float cpu_diff = cpu_target - cpu.y;

            // Only react when ball is coming towards CPU
            if (ball.vx > 0) {
                if (fabsf(cpu_diff) > 5.0f) {
                    float move = (cpu_diff > 0 ? 1.0f : -1.0f) * CPU_SPEED * CPU_REACTION * dt;
                    cpu.y += move;
                }
            } else {
                // Slowly drift back to center
                float center = PONC_HEIGHT / 2.0f - cpu.h / 2.0f;
                float diff_center = center - cpu.y;
                if (fabsf(diff_center) > 2.0f) {
                    cpu.y += (diff_center > 0 ? 1.0f : -1.0f) * CPU_SPEED * 0.3f * dt;
                }
            }

            }

            // Clamp CPU
            if (cpu.y < 0) cpu.y = 0;
            if (cpu.y + cpu.h > PONC_HEIGHT) cpu.y = PONC_HEIGHT - cpu.h;

            // ── Ball movement ──
            ball.x += ball.vx * ball.speed * dt;
            ball.y += ball.vy * ball.speed * dt;

            // Top/bottom wall bounce
            if (ball.y - BALL_SIZE / 2.0f <= 0) {
                ball.y = BALL_SIZE / 2.0f;
                ball.vy = fabsf(ball.vy);
            }
            if (ball.y + BALL_SIZE / 2.0f >= PONC_HEIGHT) {
                ball.y = PONC_HEIGHT - BALL_SIZE / 2.0f;
                ball.vy = -fabsf(ball.vy);
            }

            // ── Paddle collision ──
            // Player paddle (left)
            if (ball.vx < 0 && ball.x - BALL_SIZE / 2.0f <= player.x + player.w &&
                ball.x + BALL_SIZE / 2.0f >= player.x &&
                ball.y >= player.y && ball.y <= player.y + player.h) {

                ball.x = player.x + player.w + BALL_SIZE / 2.0f;

                // Angle based on where ball hit the paddle
                float hit_pos = (ball.y - player.y) / player.h;  // 0..1
                float angle = (hit_pos - 0.5f) * 2.0f * 60.0f * DEG2RAD;  // -60..+60 degrees
                ball.vx = cosf(angle);
                ball.vy = sinf(angle);

                // Speed up
                ball.speed += BALL_SPEED_INC;
                if (ball.speed > BALL_SPEED_MAX) ball.speed = BALL_SPEED_MAX;
                if (sound_loaded) { SetSoundPitch(pop_sound, 1.0f + (GetRandomValue(-10, 10) / 100.0f)); PlaySound(pop_sound); }
            }

            // CPU paddle (right)
            if (ball.vx > 0 && ball.x + BALL_SIZE / 2.0f >= cpu.x &&
                ball.x - BALL_SIZE / 2.0f <= cpu.x + cpu.w &&
                ball.y >= cpu.y && ball.y <= cpu.y + cpu.h) {

                ball.x = cpu.x - BALL_SIZE / 2.0f;

                float hit_pos = (ball.y - cpu.y) / cpu.h;
                float angle = (hit_pos - 0.5f) * 2.0f * 60.0f * DEG2RAD;
                ball.vx = -cosf(angle);
                ball.vy = sinf(angle);

                ball.speed += BALL_SPEED_INC;
                if (ball.speed > BALL_SPEED_MAX) ball.speed = BALL_SPEED_MAX;
                if (sound_loaded) { SetSoundPitch(pop_sound, 1.1f + (GetRandomValue(-10, 10) / 100.0f)); PlaySound(pop_sound); }
            }

            // ── Scoring ──
            if (ball.x < -BALL_SIZE) {
                // CPU scores
                score_cpu++;
                ponc_state = PONC_STATE_GOAL;
                state_timer = 0.0f;
            }
            if (ball.x > PONC_WIDTH + BALL_SIZE) {
                // Player scores
                score_p1++;
                ponc_state = PONC_STATE_GOAL;
                state_timer = 0.0f;
            }

            // Trail alpha
            ball_trail_alpha = ball.speed / BALL_SPEED_MAX;
            break;
        }
    }

    // Allow paddle movement in countdown too
    if (ponc_state == PONC_STATE_COUNTDOWN) {
        int p1_up = IsKeyDown(KEY_W);
        int p1_down = IsKeyDown(KEY_S);
        
        if (game_mode == 0) {
            if (IsKeyDown(KEY_UP)) p1_up = 1;
            if (IsKeyDown(KEY_DOWN)) p1_down = 1;
        }

        if (p1_up) {
            player.y -= PADDLE_SPEED * dt;
        }
        if (p1_down) {
            player.y += PADDLE_SPEED * dt;
        }
        
        if (game_mode == 1) {
            if (IsKeyDown(KEY_UP)) cpu.y -= PADDLE_SPEED * dt;
            if (IsKeyDown(KEY_DOWN)) cpu.y += PADDLE_SPEED * dt;
            if (cpu.y < 0) cpu.y = 0;
            if (cpu.y + cpu.h > PONC_HEIGHT) cpu.y = PONC_HEIGHT - cpu.h;
        }
        
        if (player.y < 0) player.y = 0;
        if (player.y + player.h > PONC_HEIGHT) player.y = PONC_HEIGHT - player.h;
    }
}

void ponc_draw(void) {

    // ── Background ──
    ClearBackground(PONC_BG);

    if (ponc_state == PONC_STATE_MENU) {
        DrawText(L("SELECIONE O MODO", "SELECT MODE"), PONC_WIDTH / 2 - MeasureText(L("SELECIONE O MODO", "SELECT MODE"), 40) / 2, 100, 40, WHITE);
        
        Color c1 = (game_mode == 0) ? PURPLE : WHITE;
        Color c2 = (game_mode == 1) ? PURPLE : WHITE;
        
        DrawText(L("<- 1 Jogador (Vs CPU)", "<- 1 Player (Vs CPU)"), PONC_WIDTH / 2 - 200, 250, 20, c1);
        DrawText(L("2 Jogadores ->", "2 Players ->"), PONC_WIDTH / 2 + 50, 250, 20, c2);
        
        DrawText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), PONC_WIDTH / 2 - MeasureText(L("Pressione ESQUERDA ou DIREITA para escolher", "Press LEFT or RIGHT to choose"), 20) / 2, 400, 20, DARKGRAY);
        return;
    }

    // Court area
    DrawRectangle(10, 10, PONC_WIDTH - 20, PONC_HEIGHT - 20, PONC_COURT);
    DrawRectangleLinesEx((Rectangle){ 10, 10, PONC_WIDTH - 20, PONC_HEIGHT - 20 }, 1.0f, PONC_LINE);

    // Center line (dashed)
    for (int y = 15; y < PONC_HEIGHT - 15; y += 20) {
        DrawRectangle(PONC_WIDTH / 2 - 1, y, 2, 10, PONC_LINE);
    }

    // Center square
    DrawRectangleLines(PONC_WIDTH / 2 - 40, PONC_HEIGHT / 2 - 40, 80, 80, PONC_LINE);

    // ── Scores ──
    char score_text[16];

    snprintf(score_text, sizeof(score_text), "%d", score_p1);
    int s1_w = MeasureText(score_text, 60);
    DrawText(score_text, PONC_WIDTH / 2 - s1_w - 30, 30, 60, PONC_PADDLE_P1);
    DrawText("P1", PONC_WIDTH / 2 - s1_w - 65, 60, 24, PONC_PADDLE_P1);

    snprintf(score_text, sizeof(score_text), "%d", score_cpu);
    int s2_w = MeasureText(score_text, 60);
    DrawText(score_text, PONC_WIDTH / 2 + 30, 30, 60, PONC_PADDLE_CPU);
    const char *p2_label = (game_mode == 1) ? "P2" : "CPU";
    DrawText(p2_label, PONC_WIDTH / 2 + 30 + s2_w + 15, 60, 24, PONC_PADDLE_CPU);

    // ── Paddles ──
    // Player paddle with glow
    for (int g = 2; g >= 1; g--) {
        Color glow = PONC_PADDLE_P1;
        glow.a = (unsigned char)(30 * g);
        DrawRectangle((int)player.x - g * 2, (int)player.y - g, (int)player.w + g * 4, (int)player.h + g * 2, glow);
    }
    DrawRectangle((int)player.x, (int)player.y, (int)player.w, (int)player.h, PONC_PADDLE_P1);

    // CPU paddle with glow
    for (int g = 2; g >= 1; g--) {
        Color glow = PONC_PADDLE_CPU;
        glow.a = (unsigned char)(30 * g);
        DrawRectangle((int)cpu.x - g * 2, (int)cpu.y - g, (int)cpu.w + g * 4, (int)cpu.h + g * 2, glow);
    }
    DrawRectangle((int)cpu.x, (int)cpu.y, (int)cpu.w, (int)cpu.h, PONC_PADDLE_CPU);

    if (ponc_state == PONC_STATE_PLAYING || ponc_state == PONC_STATE_COUNTDOWN) {
        const char *c1 = L("P1: W/S", "P1: W/S");
        DrawText(c1, 20, PONC_HEIGHT - 30, 15, (Color){200, 200, 200, 80});
        if (game_mode == 1) {
            const char *c2 = L("P2: Setas", "P2: Arrows");
            DrawText(c2, PONC_WIDTH - MeasureText(c2, 15) - 20, PONC_HEIGHT - 30, 15, (Color){200, 200, 200, 80});
        } else {
            const char *c2 = L("Tambem use Setas", "Also use Arrows");
            DrawText(c2, PONC_WIDTH - MeasureText(c2, 15) - 20, PONC_HEIGHT - 30, 15, (Color){200, 200, 200, 80});
        }
    }

    // ── Ball ──
    if (ponc_state == PONC_STATE_PLAYING || ponc_state == PONC_STATE_COUNTDOWN) {
        // Ball glow
        float glow_size = BALL_SIZE * (2.0f + ball_trail_alpha);
        int gs = (int)glow_size;
        DrawRectangle((int)ball.x - gs, (int)ball.y - gs, gs * 2, gs * 2, PONC_BALL_GLOW);
        int gs_inner = (int)(glow_size * 0.6f);
        DrawRectangle((int)ball.x - gs_inner, (int)ball.y - gs_inner, gs_inner * 2, gs_inner * 2,
                   (Color){ 255, 255, 200, 60 });

        // Ball
        int bs = BALL_SIZE / 2;
        DrawRectangle((int)ball.x - bs, (int)ball.y - bs, BALL_SIZE, BALL_SIZE, PONC_BALL);
    }

    // ── Countdown overlay ──
    if (ponc_state == PONC_STATE_COUNTDOWN && countdown_num > 0) {
        char cd_text[16];
        snprintf(cd_text, sizeof(cd_text), "%d", countdown_num);
        int cd_size = 100;

        // Fade effect
        float frac = state_timer - (int)state_timer;
        unsigned char alpha = (unsigned char)(255 * (1.0f - frac));
        float scale = 1.0f + frac * 0.5f;
        int scaled_size = (int)(cd_size * scale);
        int scaled_w = MeasureText(cd_text, scaled_size);

        Color cd_col = PONC_BALL;
        cd_col.a = alpha;
        DrawText(cd_text, (PONC_WIDTH - scaled_w) / 2, (PONC_HEIGHT - scaled_size) / 2, scaled_size, cd_col);
    }

    // ── Goal flash ──
    if (ponc_state == PONC_STATE_GOAL) {
        float flash = 1.0f - state_timer;
        if (flash > 0) {
            DrawRectangle(0, 0, PONC_WIDTH, PONC_HEIGHT,
                          (Color){ 255, 255, 255, (unsigned char)(40 * flash) });
        }

        const char *goal_text = L("GOL!", "GOAL!");
        int goal_size = 70;
        int goal_w = MeasureText(goal_text, goal_size);
        DrawText(goal_text, (PONC_WIDTH - goal_w) / 2, (PONC_HEIGHT - goal_size) / 2, goal_size, PONC_SCORE_COL);
    }

    // ── Win screen ──
    if (ponc_state == PONC_STATE_WIN) {
        DrawRectangle(0, 0, PONC_WIDTH, PONC_HEIGHT, (Color){0, 0, 0, 200});

        const char *p2_name = (game_mode == 1) ? "P2" : "CPU";
        const char *winner = (score_p1 >= WIN_SCORE) ? L("P1 VENCEU!", "P1 WINS!") : TextFormat(L("%s VENCEU!", "%s WINS!"), p2_name);
        int win_size = 50;
        int win_w = MeasureText(winner, win_size);
        Color win_col = (score_p1 >= WIN_SCORE) ? PONC_PADDLE_P1 : PONC_PADDLE_CPU;
        DrawText(winner, (PONC_WIDTH - win_w) / 2, PONC_HEIGHT / 2 - 80, win_size, win_col);
        
        char final_score[32];
        snprintf(final_score, sizeof(final_score), "%d  -  %d", score_p1, score_cpu);
        int fs_w = MeasureText(final_score, 40);
        DrawText(final_score, (PONC_WIDTH - fs_w) / 2, PONC_HEIGHT / 2, 40, WHITE);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        const char *hint = L("Pressione ENTER para Reiniciar", "Press ENTER to Restart");
        int hw = MeasureText(hint, 20);
        DrawText(hint, (PONC_WIDTH - hw) / 2, PONC_HEIGHT / 2 + 80, 20, (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)});
    }
}

void ponc_unload(void) {
    if (sound_loaded) UnloadSound(pop_sound);
    printf("[PONC] Game unloaded\n");
}

// ══════════════════════════════════════════════
// GAME MODULE DEFINITION
// ══════════════════════════════════════════════

Game PONC_GAME = {
    .name        = "ponC",
    .description = "O classico simulador de tenis de mesa",
    .description_en = "The classic table tennis simulator",
    .game_width  = PONC_WIDTH,
    .game_height = PONC_HEIGHT,
    .init        = ponc_init,
    .update      = ponc_update,
    .draw        = ponc_draw,
    .unload      = ponc_unload,
};
