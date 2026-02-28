#include <bits/time.h>
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "input.h"
#include "render.h"

#define FPS 60
#define FRAME_US (1000000 / FPS)

typedef enum { SCR_START, SCR_GAME, SCR_GAMEOVER } Screen;

int main(void) {
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

  GameState game;
  memset(&game, 0, sizeof game);

  struct timespec t0, t1;

  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int key = -1, k;
    while ((k = input_get_key()) != -1)
      key = k;
    switch (screen) {
    case SCR_START:
      if (key == 'w' || key == KEY_UP)
        menu_sel = (menu_sel + 2) % 3;
      if (key == 's' || key == KEY_DOWN)
        menu_sel = (menu_sel + 1) % 3;
      if (key == KEY_ENTER || key == ' ') {
        if (menu_sel == 0) {
          game_init(&game);
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

    case SCR_GAME:
      if (key == KEY_UP || key == 'w')
        game.snake.next_dir = DIR_UP;
      if (key == KEY_DOWN || key == 's')
        game.snake.next_dir = DIR_DOWN;
      if (key == KEY_LEFT || key == 'a')
        game.snake.next_dir = DIR_LEFT;
      if (key == KEY_RIGHT || key == 'd')
        game.snake.next_dir = DIR_RIGHT;
      if (key == KEY_ESC || key == 'q') {
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
        screen = SCR_GAMEOVER;
      } else {
        render_game(&game, high_score);
      }

      break;
    case SCR_GAMEOVER:
      render_game_over(last_score, high_score);
      if (key != -1) {
        if (key == 'q' || key == KEY_ESC)
          running = 0;
        else
          screen = SCR_START;
      }

      break;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long us =
        (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_nsec - t0.tv_nsec) / 1000L;

    if (us < FRAME_US) {
      struct timespec ts = {0, (FRAME_US - us) * 1000L};
      nanosleep(&ts, NULL);
    }
  }

  render_show_cursor();
  render_clear();
  input_restore();

  printf("\033[?25h");
  printf("Thanks for playing Snake! Final best: %d\n", high_score);

  return 0;
}
