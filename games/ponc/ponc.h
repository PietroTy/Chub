/**
 * @file ponc.h
 * @brief Pong game module header.
 *
 * Classic Pong: Player (left) vs CPU (right).
 * Internal resolution: 640x480 (4:3 horizontal).
 */
#ifndef PONC_H
#define PONC_H

#include "game.h"

// ── Game module struct (defined in ponc.c) ──
extern Game PONC_GAME;

// ── Lifecycle callbacks ──
void ponc_init(void);
void ponc_update(void);
void ponc_draw(void);
void ponc_unload(void);

#endif // PONC_H
