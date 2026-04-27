/**
 * @file tectris.h
 * @brief Tetris game module header.
 *
 * Classic Tetris with 8 piece types (including rare Q piece).
 * Internal resolution: 300x600 (vertical / portrait).
 */
#ifndef TECTRIS_H
#define TECTRIS_H

#include "game.h"

extern Game TECTRIS_GAME;

void tectris_init(void);
void tectris_update(void);
void tectris_draw(void);
void tectris_unload(void);

#endif // TECTRIS_H
