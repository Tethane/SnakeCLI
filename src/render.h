#ifndef RENDER_H
#define RENDER_H

#include "game.h"

void render_hide_cursor(void);
void render_show_cursor(void);
void render_clear(void);
void render_game(const GameState *g, int high_score);
void render_game_over(int score, int high_score);
void render_start_screen(int high_score, int sel);

#endif
