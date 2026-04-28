/**
 * @file game.h
 * @brief Game module interface for the Chub Game Hub.
 *
 * Every game module must define a Game struct with callbacks and metadata.
 * The hub uses this interface to manage the lifecycle of each game.
 */
#ifndef GAME_H
#define GAME_H

typedef struct Game {
    const char *name;          // Display name shown in the menu
    const char *description;   // Short description / tagline (PT)
    const char *description_en;// Short description (EN)
    int game_width;            // Internal render width (game-specific)
    int game_height;           // Internal render height (game-specific)

    void (*init)(void);        // Called once when the game is selected
    void (*update)(void);      // Called every frame while the game is active
    void (*draw)(void);        // Called every frame to draw (at internal resolution)
    void (*unload)(void);      // Called when returning to the menu
    int (*get_mode)(void);     // Returns 0 for 1P/CPU, 1 for 2P
} Game;

#endif // GAME_H
