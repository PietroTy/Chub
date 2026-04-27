/**
 * @file hub.h
 * @brief Hub state machine, menu, and frame overlay system.
 *
 * Manages transitions between the menu and active games,
 * renders the game via RenderTexture at its internal resolution,
 * and draws info overlays (title, controls, credits) in the border area.
 */
#ifndef HUB_H
#define HUB_H

#include "game.h"

// ── Application states ──
typedef enum {
    STATE_MENU,
    STATE_GAME
} AppState;

// ── Hub lifecycle ──
void hub_init(void);
void hub_update(void);
void hub_draw(void);
void hub_cleanup(void);

// ── Game management ──
void hub_select_game(int index);
void hub_return_to_menu(void);

// ── State access ──
// ── Localization & Settings ──
typedef enum {
    LANG_PT,
    LANG_EN
} Language;

Language hub_get_language(void);
void hub_set_language(Language lang);
bool hub_get_mute(void);
void hub_set_mute(bool mute);

// Helper macro for game-side localization
#define L(pt, en) (hub_get_language() == LANG_PT ? pt : en)

AppState hub_get_state(void);
Game *hub_get_current_game(void);

#endif // HUB_H

int hub_load_score(int game_id);
void hub_save_score(int game_id, int score);
