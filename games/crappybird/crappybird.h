/**
 * @file crappybird.h
 * @brief Crappybird game module header.
 *
 * Flappy bird clone with pixel-art birds, day/night cycle,
 * score tracking, and unlockable bird skins.
 * Internal resolution: 600x600 (square).
 */
#ifndef CRAPPYBIRD_H
#define CRAPPYBIRD_H

#include "game.h"

extern Game CRAPPYBIRD_GAME;

void crappybird_init(void);
void crappybird_update(void);
void crappybird_draw(void);
void crappybird_unload(void);

#endif // CRAPPYBIRD_H
