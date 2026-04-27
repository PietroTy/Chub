/**
 * @file snacke.h
 * @brief Snake game module header.
 *
 * Classic snacke with apples, grapes, oranges, and victory condition.
 * Internal resolution: 600x600 (square).
 */
#ifndef SNACKE_H
#define SNACKE_H

#include "game.h"

extern Game SNACKE_GAME;

void snacke_init(void);
void snacke_update(void);
void snacke_draw(void);
void snacke_unload(void);

#endif // SNACKE_H
