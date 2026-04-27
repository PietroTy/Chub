/**
 * @file chub_colors.h
 * @brief Custom color definitions used across Chub game modules.
 *
 * These colors were originally defined in modified raylib.h headers
 * of individual game projects. Centralized here so the standard
 * raylib.h stays untouched.
 *
 * Include this header in any game module that uses these colors.
 */
#ifndef CHUB_COLORS_H
#define CHUB_COLORS_H

#include "raylib/raylib.h"

// ── Custom colors (used by Crappybird, snaCke, teCtris) ──
#ifndef THEGRAY
#define THEGRAY    CLITERAL(Color){ 30, 30, 30, 255 }
#endif

#ifndef DEEPBLUE
#define DEEPBLUE   CLITERAL(Color){ 0, 0, 172, 255 }
#endif

#ifndef DARKYELLOW
#define DARKYELLOW CLITERAL(Color){ 203, 200, 0, 255 }
#endif

#ifndef DARKSKY
#define DARKSKY    CLITERAL(Color){ 52, 141, 255, 255 }
#endif

// Note: DARKORANGE conflicts with raylib's built-in ORANGE.
// Our custom is a deeper orange-red.
#ifndef DARKORANGE
#define DARKORANGE CLITERAL(Color){ 255, 61, 0, 255 }
#endif

#ifndef DARKRED
#define DARKRED    CLITERAL(Color){ 130, 41, 55, 255 }
#endif

#endif // CHUB_COLORS_H
