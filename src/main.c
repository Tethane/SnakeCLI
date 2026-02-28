#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "input.h"
#include "render.h"

#define FPS 60
#define FRAME_NS (1000000000L / FPS)

/* Minimum playable board size */
#define MIN_BOARD_W 12
#define MIN_BOARD_H 8

typedef enum { SCR_START, SCR_GAME, SCR_GAMEOVER } Screen;

int main(int argc, char *argv[]) {
  /* ── Board dimensions from argv (set by bash wrapper via tput) ── */
  int term_cols = (argc >= 2) ? atoi(argv[1]) : 80;
  int term_rows = (argc >= 3) ? atoi(argv[2]) : 24;

  /* Play area: subtract borders (2) + side padding (2) for width,
     top border + bottom border + HUD row (3) for height            */
  int bw = (term_cols - 4) / 2;
  int bh = term_rows - 3;
  if (bw < MIN_BOARD_W)
    bw = MIN_BOARD_W;
  if (bh < MIN_BOARD_H)
    bh = MIN_BOARD_H;
  game_set_dimensions(bw, bh);

  /* ── Cache directory from environment ───────────────────────── */
  const char *cache = getenv("SNAKE_CACHE");
  if (!cache || cache[0] == '\0')
    cache = "/tmp/snake_cache";
  game_set_cache(cache);

  input_init();
  render_hide_cursor();
  render_clear();

  Screen screen = SCR_START;
  int menu_sel = 0;
  int high_score = score_load();
  int last_score = 0;
  int running = 1;

  /* game_over_drawn: ensures we clear only once on first entry    */
  int game_over_drawn = 0;

  GameState game;
  memset(&game, 0, sizeof game);

  struct timespec t0, t1;

  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* Drain key queue; act on the latest meaningful key */
    int key = -1, k;
    while ((k = input_get_key()) != -1)
      key = k;

    switch (screen) {

    /* ── Start menu ─────────────────────────────────────────── */
    case SCR_START:
      if (key == 'w' || key == KEY_UP)
        menu_sel = (menu_sel + 2) % 3;
      if (key == 's' || key == KEY_DOWN)
        menu_sel = (menu_sel + 1) % 3;
      if (key == KEY_ENTER || key == ' ') {
        if (menu_sel == 0) {
          game_init(&game);
          render_clear();
          screen = SCR_GAME;
        } else if (menu_sel == 2) {
          running = 0;
        }
      }
      if (key == 'q' || key == KEY_ESC)
        running = 0;
      if (running && screen == SCR_START)
        render_start_screen(high_score, menu_sel);
      break;

    /* ── Gameplay ───────────────────────────────────────────── */
    case SCR_GAME:
      if (key == KEY_UP || key == 'w')
        game.snake.next_dir = DIR_UP;
      if (key == KEY_DOWN || key == 's')
        game.snake.next_dir = DIR_DOWN;
      if (key == KEY_LEFT || key == 'a')
        game.snake.next_dir = DIR_LEFT;
      if (key == KEY_RIGHT || key == 'd')
        game.snake.next_dir = DIR_RIGHT;
      if (key == 'q' || key == KEY_ESC) {
        render_clear();
        screen = SCR_START;
        break;
      }

      game_update(&game);

      if (!game.alive) {
        last_score = game.score;
        if (last_score > high_score) {
          high_score = last_score;
          score_save(high_score);
        }
        render_clear(); /* wipe the game board first */
        game_over_drawn = 0;
        screen = SCR_GAMEOVER;
      } else {
        render_game(&game, high_score);
      }
      break;

    /* ── Game over ──────────────────────────────────────────── */
    case SCR_GAMEOVER:
      if (!game_over_drawn) {
        render_game_over(last_score, high_score);
        game_over_drawn = 1;
      }
      if (key != -1) {
        render_clear(); /* wipe game-over before next screen */
        if (key == 'q' || key == KEY_ESC) {
          running = 0;
        } else {
          screen = SCR_START;
          game_free(&game);
        }
      }
      break;
    }

    /* ── Frame-rate limiter (nanosleep) ─────────────────────── */
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long ns = (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec);
    if (ns < FRAME_NS) {
      struct timespec ts = {0, FRAME_NS - ns};
      nanosleep(&ts, NULL);
    }
  }

  /* ── Teardown ────────────────────────────────────────────────── */
  game_free(&game);
  render_show_cursor();
  render_clear();
  input_restore();

  printf("Thanks for playing Snake! Final best: %d\n", high_score);
  return 0;
}
