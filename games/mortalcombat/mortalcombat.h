/**
 * @file mortalcombat.h
 * @brief Fighting game module header.
 *
 * 2-player mortalcombat game inspired by the mortalcombat-game-master project.
 * Uses colored rectangles and geometric shapes for visuals (Chub style).
 * Features: movement, jumping, crouching, 3 punch/kick attacks, blocking,
 *           projectile (Hadouken), health bar, round system, and win screen.
 *
 * Player 1: WASD + U/I/O (punch) + J/K/L (kick)
 * Player 2: Arrows + Numpad 1/2/3 (punch) + 4/5/6 (kick)
 * Internal resolution: 640x480
 */
#ifndef MORTALCOMBAT_H
#define MORTALCOMBAT_H

#include "game.h"

extern Game MORTALCOMBAT_GAME;

void mortalcombat_init(void);
void mortalcombat_update(void);
void mortalcombat_draw(void);
void mortalcombat_unload(void);

#endif // MORTALCOMBAT_H
