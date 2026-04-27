/**
 * @file registry.h
 * @brief Central registry of all available game modules.
 *
 * To add a new game:
 *   1. Create your game module in games/yourgame/
 *   2. Add an extern declaration for your Game struct here
 *   3. Add the pointer to the games[] array in registry.c
 *   4. Recompile (game_count is auto-calculated via sizeof)
 */
#ifndef REGISTRY_H
#define REGISTRY_H

#include "game.h"

// ── Extern declarations for all game modules ──
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

// ── Registry access ──
extern Game *games[];
extern int game_count;

#endif // REGISTRY_H
