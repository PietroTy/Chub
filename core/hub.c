/**
 * @file hub.c
 * @brief Hub state machine, menu rendering, and frame overlay.
 *
 * The hub manages two states:
 *   STATE_MENU — interactive game selection menu
 *   STATE_GAME — active game rendered via RenderTexture at internal resolution
 *
 * During STATE_GAME, the game is drawn into a RenderTexture at its own
 * resolution, then scaled and centered on the canvas. The surrounding
 * border area displays project info, game name, controls, and credits.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "raylib/raylib.h"
#include "hub.h"
#include "registry.h"

// ── Colors (purple premium palette) ──
#define COL_BG_DARK       (Color){ 20, 10, 30, 255 }
#define COL_BG_CARD       (Color){ 35, 18, 50, 255 }
#define COL_BG_CARD_HOVER (Color){ 55, 25, 80, 255 }
#define COL_ACCENT        (Color){ 180, 100, 255, 255 }
#define COL_ACCENT_DIM    (Color){ 110, 60, 160, 255 }
#define COL_TEXT_PRIMARY   (Color){ 245, 235, 255, 255 }
#define COL_TEXT_SECONDARY (Color){ 180, 150, 210, 255 }
#define COL_TEXT_MUTED     (Color){ 120, 90, 150, 255 }
#define COL_BORDER         (Color){ 80, 45, 110, 255 }
#define COL_NEON_GLOW      (Color){ 180, 100, 255, 60 }
#define COL_OVERLAY_BG     (Color){ 15, 5, 25, 255 }

// ── State ──
static AppState state = STATE_MENU;
static Game *current_game = NULL;
static RenderTexture2D game_render_target;
static bool render_target_loaded = false;

static Language current_lang = LANG_EN;
static bool is_muted = false;

// ── Settings Persistence ──
static void save_settings(void) {
    FILE *f = fopen("settings.dat", "wb");
    if (f) {
        int l = (int)current_lang;
        fwrite(&l, sizeof(int), 1, f);
        fwrite(&is_muted, sizeof(bool), 1, f);
        fclose(f);
    }
}

static void load_settings(void) {
    FILE *f = fopen("settings.dat", "rb");
    if (f) {
        int l;
        if (fread(&l, sizeof(int), 1, f)) current_lang = (Language)l;
        fread(&is_muted, sizeof(bool), 1, f);
        fclose(f);
        SetMasterVolume(is_muted ? 0.0f : 1.0f);
    } else {
        current_lang = LANG_EN;
        is_muted = false;
    }
}

Language hub_get_language(void) { return current_lang; }
void hub_set_language(Language lang) { current_lang = lang; save_settings(); }
bool hub_get_mute(void) { return is_muted; }
void hub_set_mute(bool mute) { 
    is_muted = mute; 
    SetMasterVolume(is_muted ? 0.0f : 1.0f);
    save_settings();
}

// ── UI Helpers ──
static void draw_settings_buttons(bool overlay) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    
    // Bottom-left area
    int x = 20;
    int y = sh - 50;
    
    Rectangle btn_lang = { (float)x, (float)y, 40, 30 };
    Rectangle btn_sound = { (float)x + 50, (float)y, 40, 30 };
    
    Vector2 mp = GetMousePosition();
    bool hover_lang = CheckCollisionPointRec(mp, btn_lang);
    bool hover_sound = CheckCollisionPointRec(mp, btn_sound);
    
    // Lang Button
    DrawRectangleRec(btn_lang, hover_lang ? COL_BG_CARD_HOVER : COL_BG_CARD);
    DrawRectangleLinesEx(btn_lang, 1, hover_lang ? COL_ACCENT : COL_BORDER);
    const char *lang_str = (current_lang == LANG_PT) ? "PT" : "EN";
    DrawText(lang_str, x + 10, y + 7, 18, COL_TEXT_PRIMARY);
    
    // Sound Button
    DrawRectangleRec(btn_sound, hover_sound ? COL_BG_CARD_HOVER : COL_BG_CARD);
    DrawRectangleLinesEx(btn_sound, 1, hover_sound ? COL_ACCENT : COL_BORDER);
    
    // New Pixel-art Speaker Icon
    Color icon_col = is_muted ? RED : COL_TEXT_PRIMARY;
    
    // Speaker Base (Square)
    DrawRectangle(x + 50 + 8, y + 11, 8, 8, icon_col);
    // Speaker Front (Tall Rectangle)
    DrawRectangle(x + 50 + 16, y + 6, 5, 18, icon_col);
    
    // Sound Bar (only if not muted)
    if (!is_muted) {
        DrawRectangle(x + 50 + 25, y + 10, 7, 10, COL_ACCENT);
    } else {
        // Red X for muted
        DrawLine(x + 50 + 24, y + 10, x + 50 + 32, y + 20, RED);
        DrawLine(x + 50 + 32, y + 10, x + 50 + 24, y + 20, RED);
    }

    // (Click logic moved to hub_update)
}

static void update_settings_buttons(void) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    
    int sh = GetScreenHeight();
    int x = 20;
    int y = sh - 50;
    Rectangle btn_lang = { (float)x, (float)y, 40, 30 };
    Rectangle btn_sound = { (float)x + 50, (float)y, 40, 30 };
    
    Vector2 mp = GetMousePosition();
    if (CheckCollisionPointRec(mp, btn_lang)) hub_set_language(current_lang == LANG_PT ? LANG_EN : LANG_PT);
    if (CheckCollisionPointRec(mp, btn_sound)) hub_set_mute(!is_muted);
}

// ── Helpers ──

static void DrawTextWrapped(const char *text, float x, float y, float maxWidth, float fontSize, Color color) {
    if (!text) return;
    Font font = GetFontDefault();
    float spacing = fontSize / 10.0f;
    
    char buffer[256];
    strncpy(buffer, text, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    
    char *words[64];
    int wordCount = 0;
    char *word = strtok(buffer, " ");
    while (word && wordCount < 64) {
        words[wordCount++] = word;
        word = strtok(NULL, " ");
    }
    
    char line[256] = "";
    float cursorY = y;
    
    for (int i = 0; i < wordCount; i++) {
        char nextLine[256];
        if (line[0] == '\0') strcpy(nextLine, words[i]);
        else snprintf(nextLine, sizeof(nextLine), "%s %s", line, words[i]);
        
        Vector2 size = MeasureTextEx(font, nextLine, fontSize, spacing);
        if (size.x > maxWidth && line[0] != '\0') {
            // Draw current line centered
            Vector2 lineSize = MeasureTextEx(font, line, fontSize, spacing);
            DrawTextEx(font, line, (Vector2){ x + (maxWidth - lineSize.x)/2.0f, cursorY }, fontSize, spacing, color);
            cursorY += fontSize + 4;
            strcpy(line, words[i]);
        } else {
            strcpy(line, nextLine);
        }
    }
    
    if (line[0] != '\0') {
        Vector2 lineSize = MeasureTextEx(font, line, fontSize, spacing);
        DrawTextEx(font, line, (Vector2){ x + (maxWidth - lineSize.x)/2.0f, cursorY }, fontSize, spacing, color);
    }
}

// ── HUB Main Functions ──
static int hovered_index = 0;
static float carousel_offset = 0.0f;
static float menu_anim_time = 0.0f;

// ── Forward declarations ──
static void draw_menu(void);
static void draw_game_frame(void);
static void update_menu(void);
static void update_game(void);

// ── Hub lifecycle ──


#ifdef PLATFORM_WEB
#include <emscripten.h>
#endif

#define MAX_SCORES 10
static int highscores[MAX_SCORES];

void hub_load_all_scores(void) {
#ifndef PLATFORM_WEB
    FILE *f = fopen("chub_scores.dat", "rb");
    if (f) {
        fread(highscores, sizeof(int), MAX_SCORES, f);
        fclose(f);
    } else {
        for (int i = 0; i < MAX_SCORES; i++) highscores[i] = 0;
    }
#endif
}

void hub_save_all_scores(void) {
#ifndef PLATFORM_WEB
    FILE *f = fopen("chub_scores.dat", "wb");
    if (f) {
        fwrite(highscores, sizeof(int), MAX_SCORES, f);
        fclose(f);
    }
#endif
}

int hub_load_score(int game_id) {
#ifdef PLATFORM_WEB
    return EM_ASM_INT({
        var val = window.localStorage.getItem('chub_score_' + $0);
        return val ? parseInt(val) : 0;
    }, game_id);
#else
    if (game_id < 0 || game_id >= MAX_SCORES) return 0;
    return highscores[game_id];
#endif
}

void hub_save_score(int game_id, int score) {
#ifdef PLATFORM_WEB
    EM_ASM({
        window.localStorage.setItem('chub_score_' + $0, $1);
    }, game_id, score);
#else
    if (game_id < 0 || game_id >= MAX_SCORES) return;
    highscores[game_id] = score;
    hub_save_all_scores();
#endif
}

void hub_init(void) {
    load_settings();
    hub_load_all_scores();
    state = STATE_MENU;
    current_game = NULL;
    hovered_index = 0;
    carousel_offset = 0.0f;
    menu_anim_time = 0.0f;
    render_target_loaded = false;
}

void hub_update(void) {
    menu_anim_time += GetFrameTime();
    
    // Global Shortcuts
    if (IsKeyPressed(KEY_M)) hub_set_mute(!is_muted);
    if (IsKeyPressed(KEY_L)) hub_set_language(current_lang == LANG_PT ? LANG_EN : LANG_PT);

    if (state == STATE_MENU) {
        update_menu();
    } else if (state == STATE_GAME) {
        update_game();
    }
    
    update_settings_buttons();
}

void hub_draw(void) {
    if (state == STATE_MENU) {
        ClearBackground(COL_BG_DARK);
        draw_menu();
        draw_settings_buttons(false);
    } else if (state == STATE_GAME) {
        ClearBackground(COL_OVERLAY_BG);
        draw_game_frame();
        draw_settings_buttons(true);
    }
}

void hub_cleanup(void) {
    if (current_game != NULL) {
        current_game->unload();
        current_game = NULL;
    }
    if (render_target_loaded) {
        UnloadRenderTexture(game_render_target);
        render_target_loaded = false;
    }
}

// ── Game management ──

void hub_select_game(int index) {
    if (index < 0 || index >= game_count) return;

    current_game = games[index];

    // Create RenderTexture at the game's internal resolution
    if (render_target_loaded) {
        UnloadRenderTexture(game_render_target);
    }
    game_render_target = LoadRenderTexture(current_game->game_width, current_game->game_height);
    render_target_loaded = true;

    current_game->init();
    state = STATE_GAME;
}

void hub_return_to_menu(void) {
    if (current_game != NULL) {
        current_game->unload();
        current_game = NULL;
    }
    if (render_target_loaded) {
        UnloadRenderTexture(game_render_target);
        render_target_loaded = false;
    }
    state = STATE_MENU;
}

AppState hub_get_state(void) {
    return state;
}

Game *hub_get_current_game(void) {
    return current_game;
}

// ══════════════════════════════════════════════
// MENU
// ══════════════════════════════════════════════

static void update_menu(void) {
    int screen_w = GetScreenWidth();

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        hovered_index = (hovered_index + 1) % game_count;
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        hovered_index = (hovered_index - 1 + game_count) % game_count;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        hub_select_game(hovered_index);
        return;
    }

    // Smooth offset interpolation for carousel
    float target = (float)hovered_index;
    float diff = target - carousel_offset;
    if (diff > game_count / 2.0f) carousel_offset += game_count;
    if (diff < -game_count / 2.0f) carousel_offset -= game_count;
    
    carousel_offset += (target - carousel_offset) * 10.0f * GetFrameTime();

    // Mouse click handling for carousel
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        int screen_h = GetScreenHeight();
        // Ignore clicks in the bottom-left settings button area
        if (mouse.x < 120 && mouse.y > screen_h - 70) return;
        
        int card_w = 280;
        int center_x = screen_w / 2;
        if (mouse.x > center_x - card_w/2 && mouse.x < center_x + card_w/2) {
            hub_select_game(hovered_index);
        } else if (mouse.x < center_x - card_w/2) {
            hovered_index = (hovered_index - 1 + game_count) % game_count;
        } else {
            hovered_index = (hovered_index + 1) % game_count;
        }
    }
}

static void draw_menu(void) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();

    // ── Background subtle pattern ──
    for (int i = 0; i < screen_w; i += 40) {
        int alpha = 8 + (int)(4.0f * sinf(menu_anim_time * 0.5f + i * 0.02f));
        DrawLine(i, 0, i, screen_h, (Color){ 160, 100, 255, (unsigned char)alpha });
    }
    for (int j = 0; j < screen_h; j += 40) {
        int alpha = 6 + (int)(3.0f * sinf(menu_anim_time * 0.3f + j * 0.015f));
        DrawLine(0, j, screen_w, j, (Color){ 160, 100, 255, (unsigned char)alpha });
    }

    // ── Title "Chub" ──
    const char *title = L("Chub Game Hub", "Chub Game Hub");
    int title_size = 80;
    int title_w = MeasureText(title, title_size);
    int title_x = (screen_w - title_w) / 2;
    int title_y = 60;
    float glow_pulse = 0.6f + 0.4f * sinf(menu_anim_time * 2.0f);
    DrawText(title, title_x, title_y, title_size, COL_ACCENT);

    const char *subtitle = L("Game Hub para jogos em C", "C-based Game Hub");
    int sub_size = 24;
    int sub_w = MeasureText(subtitle, sub_size);
    DrawText(subtitle, (screen_w - sub_w) / 2, title_y + title_size + 5, sub_size, COL_TEXT_SECONDARY);

    // ── Carousel ──
    int card_w = 300;
    int card_h = 400;
    int card_gap = 40;
    int center_x = screen_w / 2;
    int cy = screen_h / 2 + 20;

    for (int i = 0; i < game_count; i++) {
        float offset = i - carousel_offset;
        while (offset > game_count / 2.0f) offset -= game_count;
        while (offset < -game_count / 2.0f) offset += game_count;

        if (offset < -3.0f || offset > 3.0f) continue;

        float draw_x = center_x + offset * (card_w + card_gap);
        
        float dist = fabsf(offset);
        float scale = 1.0f - (dist * 0.15f);
        if (scale < 0.5f) scale = 0.5f;
        float alpha_f = 1.0f - (dist * 0.4f);
        if (alpha_f < 0.0f) alpha_f = 0.0f;
        
        int w = (int)(card_w * scale);
        int h = (int)(card_h * scale);
        int x = (int)(draw_x - w / 2);
        int y = (int)(cy - h / 2);

        Rectangle card_rect = { (float)x, (float)y, (float)w, (float)h };
        bool is_center = (i == hovered_index);

        Color bg = is_center ? COL_BG_CARD_HOVER : COL_BG_CARD;
        bg.a = (unsigned char)(bg.a * alpha_f);
        DrawRectangleRec(card_rect, bg);

        Color border = is_center ? COL_ACCENT : COL_BORDER;
        border.a = (unsigned char)(border.a * alpha_f);
        DrawRectangleLinesEx(card_rect, is_center ? 3.0f : 2.0f, border);

        if (is_center) {
            Color glow = COL_NEON_GLOW;
            glow.a = (unsigned char)(glow.a * glow_pulse);
            DrawRectangleRec(card_rect, glow);
        }

        if (alpha_f > 0.1f) {
            char num[16];
            snprintf(num, sizeof(num), "%d", i + 1);
            int badge_size = (int)(40 * scale);
            Color num_col = is_center ? COL_ACCENT : COL_ACCENT_DIM;
            num_col.a = (unsigned char)(num_col.a * alpha_f);
            DrawText(num, x + 20, y + 20, badge_size, num_col);

            int name_size = (int)(48 * scale);
            int nw = MeasureText(games[i]->name, name_size);
            // Shrink name if it's exceptionally long (e.g. DoodleJump on smaller screens)
            while (nw > w - 20 && name_size > 15) {
                name_size--;
                nw = MeasureText(games[i]->name, name_size);
            }
            Color name_col = COL_TEXT_PRIMARY;
            name_col.a = (unsigned char)(name_col.a * alpha_f);
            DrawText(games[i]->name, x + (w - nw)/2, y + h/2 - 60, name_size, name_col);

            if (is_center && games[i]->description) {
                float desc_size = 24.0f * scale;
                float padding = 20.0f * scale;
                DrawTextWrapped(L(games[i]->description, games[i]->description_en), (float)x + padding, (float)y + h/2 + 10, (float)w - padding * 2, desc_size, COL_TEXT_SECONDARY);
            }
        }
    }

    // ── Arrows ──
    float arrow_pulse = 0.5f + 0.5f * sinf(menu_anim_time * 5.0f);
    Color arrow_col = COL_ACCENT;
    arrow_col.a = (unsigned char)(200 + 55 * arrow_pulse);
    DrawText("<", center_x - card_w/2 - 60, cy - 20, 40, arrow_col);
    DrawText(">", center_x + card_w/2 + 40, cy - 20, 40, arrow_col);

    // ── Footer ──
    const char *footer = L("Use Setas Esquerda/Direita e ENTER", "Use Left/Right Arrows and ENTER");
    int footer_size = 22;
    int footer_w = MeasureText(footer, footer_size);
    float footer_alpha = 0.4f + 0.3f * sinf(menu_anim_time * 1.0f);
    Color footer_col = COL_TEXT_MUTED;
    footer_col.a = (unsigned char)(255 * footer_alpha);
    DrawText(footer, (screen_w - footer_w) / 2, screen_h - 75, footer_size, footer_col);
    DrawText("@PietroTy - 2026", (screen_w - MeasureText("@PietroTy - 2026", 28)) / 2, screen_h - 40, 28, COL_TEXT_MUTED);
}

// ══════════════════════════════════════════════
// GAME FRAME (RenderTexture + overlay)
// ══════════════════════════════════════════════

static void update_game(void) {
    if (current_game == NULL) return;

    // ESC returns to menu
    if (IsKeyPressed(KEY_ESCAPE)) {
        hub_return_to_menu();
        return;
    }

    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    int info_bar_h = 36;
    int side_panel_w = (screen_w > 800) ? 200 : 0;
    int available_w = screen_w - side_panel_w;
    int available_h = screen_h - info_bar_h; // Removed bottom bar space

    float scale_x = (float)available_w / (float)current_game->game_width;
    float scale_y = (float)available_h / (float)current_game->game_height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    int draw_w = (int)(current_game->game_width * scale);
    int draw_h = (int)(current_game->game_height * scale);
    int draw_x = (screen_w - draw_w) / 2;
    int draw_y = info_bar_h + (available_h - draw_h) / 2;

    SetMouseOffset(-draw_x, -draw_y);
    SetMouseScale(1.0f / scale, 1.0f / scale);

    current_game->update();

    SetMouseOffset(0, 0);
    SetMouseScale(1.0f, 1.0f);
}

static void draw_game_frame(void) {
    if (current_game == NULL || !render_target_loaded) return;

    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();

    // ── Render game into its own RenderTexture ──
    BeginTextureMode(game_render_target);
        current_game->draw();
    EndTextureMode();

    // ── Calculate scaling and centering ──
    // Reserve space for info bars (top and bottom)
    int info_bar_h = 36;
    int side_panel_w = (screen_w > 800) ? 200 : 0;
    int available_w = screen_w - side_panel_w;
    int available_h = screen_h - info_bar_h; // Removed bottom bar space

    float scale_x = (float)available_w / (float)current_game->game_width;
    float scale_y = (float)available_h / (float)current_game->game_height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    // Limit scale to integer multiples for pixel-perfect rendering (optional)
    // If you want pixel-perfect: scale = floorf(scale);
    // For now, allow fractional scaling for better fit

    int draw_w = (int)(current_game->game_width * scale);
    int draw_h = (int)(current_game->game_height * scale);
    int draw_x = (screen_w - draw_w) / 2;
    int draw_y = info_bar_h + (available_h - draw_h) / 2;

    // ── Draw the game texture (flipped Y because RenderTexture is flipped) ──
    Rectangle src = { 0, 0, (float)current_game->game_width, -(float)current_game->game_height };
    Rectangle dst = { (float)draw_x, (float)draw_y, (float)draw_w, (float)draw_h };
    DrawTexturePro(game_render_target.texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);

    // ── Border around game area ──
    DrawRectangleLinesEx((Rectangle){ (float)(draw_x - 2), (float)(draw_y - 2),
                         (float)(draw_w + 4), (float)(draw_h + 4) }, 2.0f, COL_ACCENT_DIM);

    // ── Top info bar ──
    DrawRectangle(0, 0, screen_w, info_bar_h, (Color){ 12, 12, 20, 240 });
    DrawLine(0, info_bar_h, screen_w, info_bar_h, COL_BORDER);

    // Left: Chub logo
    DrawText("Chub", 12, 8, 22, COL_ACCENT);

    // Center: game name
    if (current_game->name) {
        int name_w = MeasureText(current_game->name, 20);
        DrawText(current_game->name, (screen_w - name_w) / 2, 10, 20, COL_TEXT_PRIMARY);
    }

    // Right: ESC hint
    const char *esc_text = "[ESC] Menu";
    int esc_w = MeasureText(esc_text, 16);
    DrawText(esc_text, screen_w - esc_w - 12, 12, 16, COL_TEXT_MUTED);

    // (Bottom info bar removed)

    // ── Side panels (if there's space) ──
    int side_left_w = draw_x - 2;
    int side_right_x = draw_x + draw_w + 2;
    int side_right_w = screen_w - side_right_x;

    // Left side: credits
    if (side_left_w > 80) {
        int text_x = 12;
        int text_y = draw_y + 10;
        DrawText("Game Hub", text_x, text_y, 32, COL_TEXT_MUTED);
        DrawText("v1.0", text_x, text_y + 40, 24, COL_TEXT_MUTED);
        DrawText("@PietroTy 2026", text_x, text_y + 70, 22, COL_TEXT_MUTED);

        const char *thanks = NULL;
        if (strcmp(current_game->name, "Crappy bird") == 0) thanks = "Rodrigo Garcia";
        else if (strcmp(current_game->name, "snaCke") == 0) thanks = "Davi Beli";
        else if (strcmp(current_game->name, "Candy Crush") == 0 ||
                 strcmp(current_game->name, "Crazy jump") == 0 ||
                 strcmp(current_game->name, "mortal Combat") == 0 ||
                 strcmp(current_game->name, "spaCe invaders") == 0 ||
                 strcmp(current_game->name, "C ball pool") == 0 ||
                 strcmp(current_game->name, "solitaire Cpider") == 0) {
            thanks = "David Buzatto";
        }

        if (thanks) {
            DrawText(L("Agradecimento:", "Credits:"), text_x, text_y + 110, 20, COL_TEXT_SECONDARY);
            DrawText(thanks, text_x, text_y + 135, 20, COL_TEXT_MUTED);
        }
    }

    // Right side: controls hint
    if (side_right_w > 40) {
        int text_x = side_right_x + 12;
        int text_y = draw_y + 10;
        DrawText(L("Controles", "Controls"), text_x, text_y, 32, COL_TEXT_SECONDARY);
        
        const char *ctrl1 = "";
        const char *ctrl2 = "";
        const char *ctrl3 = "";
        const char *ctrl4 = "";
        
        if (strcmp(current_game->name, "ponC") == 0) { 
            ctrl1 = "W/S ou Setas"; 
            ctrl2 = "(P1 e P2)";
        }
        else if (strcmp(current_game->name, "Crappy bird") == 0) { 
            ctrl1 = "Espaco: pular"; 
            ctrl2 = "S: Loja";
        }
        else if (strcmp(current_game->name, "snaCke") == 0) { 
            ctrl1 = "Setas ou WASD"; 
            ctrl2 = "p/ mover"; 
        }
        else if (strcmp(current_game->name, "teCtris") == 0) { 
            ctrl1 = "Setas: move"; 
            ctrl2 = "Cima: gira"; 
            ctrl3 = "Espaco: Drop"; 
        }
        else if (strcmp(current_game->name, "mortal Combat") == 0) { 
            ctrl1 = "P1: WASD+UIOP"; 
            ctrl2 = "P2: Setas+Num"; 
            ctrl3 = "P/Vs CPU usa";
            ctrl4 = "ambos p/ P1";
        }
        else if (strcmp(current_game->name, "Ctron") == 0) { 
            ctrl1 = "P1: WASD"; 
            ctrl2 = "P2: Setas"; 
        }
        else if (strcmp(current_game->name, "spaCe invaders") == 0) { 
            ctrl1 = "AD / Setas"; 
            ctrl2 = "Espaco: tiro"; 
        }
        else if (strcmp(current_game->name, "Crazy jump") == 0) { 
            ctrl1 = "AD ou Setas"; 
        }
        else if (strcmp(current_game->name, "C ball pool") == 0) {
            ctrl1 = "Mouse: arrasta";
            ctrl2 = "e solta taco";
        }
        else if (strcmp(current_game->name, "Candy Crush") == 0) {
            ctrl1 = "Mouse: clique";
            ctrl2 = "p/ trocar";
        }
        else if (strcmp(current_game->name, "solitaire Cpider") == 0) {
            ctrl1 = L("Mouse: arrasta", "Mouse: drag");
            ctrl2 = L("R: Reiniciar", "R: Restart");
            ctrl3 = L("DblClick: envia", "DblClick: send");
            ctrl4 = L("para a base", "to foundation");
        }

        if (ctrl1[0]) DrawText(ctrl1, text_x, text_y + 45, 20, COL_TEXT_MUTED);
        if (ctrl2[0]) DrawText(ctrl2, text_x, text_y + 70, 20, COL_TEXT_MUTED);
        if (ctrl3[0]) DrawText(ctrl3, text_x, text_y + 95, 20, COL_TEXT_MUTED);
        if (ctrl4[0]) DrawText(ctrl4, text_x, text_y + 120, 20, COL_TEXT_MUTED);
    }
}
