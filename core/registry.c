/**
 * @file registry.c
 * @brief Central registry of all game modules.
 *
 * Add new games here. Each game must define a Game struct
 * (extern declared in registry.h) and be added to the games[] array.
 * game_count is auto-calculated via sizeof.
 */
#include "game.h"
#include "registry.h"

// ── Extern game structs (defined in each game module) ──
extern Game PONC_GAME;
extern Game CRAPPYBIRD_GAME;
extern Game SNACKE_GAME;
extern Game TECTRIS_GAME;
extern Game MORTALCOMBAT_GAME;
extern Game CTRON_GAME;
extern Game SPACEINVADERS_GAME;
extern Game CRAZYJUMP_GAME;
extern Game CBALLPOOL_GAME;
extern Game CANDYCRUSH_GAME;
extern Game SOLITAIRECPIDER_GAME;

// ── Registry array — order determines menu display order ──
Game *games[] = {
    &PONC_GAME,
    &CRAPPYBIRD_GAME,
    &SNACKE_GAME,
    &TECTRIS_GAME,
    &MORTALCOMBAT_GAME,
    &CTRON_GAME,
    &SPACEINVADERS_GAME,
    &CRAZYJUMP_GAME,
    &CBALLPOOL_GAME,
    &CANDYCRUSH_GAME,
    &SOLITAIRECPIDER_GAME,
};

int game_count = sizeof(games) / sizeof(games[0]);
