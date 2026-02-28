#ifndef RENDER_H
#define RENDER_H

#include "game.h"

/* Call once after board dimensions are known */
void render_set_term_size(int cols, int rows);

void render_hide_cursor(void);
void render_show_cursor(void);
void render_clear(void);
void render_game(const GameState *g, int high_score);
void render_start_screen(int high_score, int sel);
void render_game_over(int score, int high_score);

#define MENU_COUNT 2 /* "New Game" and "Quit" */

#endif
