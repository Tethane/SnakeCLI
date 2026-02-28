#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "input.h"
#include "render.h"

#define FPS 60
#define FRAME_NS (1000000000L / FPS)

#define MIN_BOARD_W 12
#define MIN_BOARD_H 8

typedef enum { SCR_START, SCR_GAME, SCR_GAMEOVER } Screen;

/* ── SIGWINCH ────────────────────────────────────────────────
   Set atomically from signal handler; consumed in main loop. */
static volatile sig_atomic_t resize_pending = 0;
static void on_sigwinch(int sig) {
  (void)sig;
  resize_pending = 1;
}

/* ── Query terminal size via ioctl ──────────────────────────── */
static void query_term_size(int *cols, int *rows) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 &&
      ws.ws_row > 0) {
    *cols = (int)ws.ws_col;
    *rows = (int)ws.ws_row;
  }
  /* else: leave caller's defaults unchanged */
}

/* ── Apply new terminal size ─────────────────────────────────
   Returns 1 if a running game must be aborted (board changed). */
static int apply_resize(int *term_cols, int *term_rows) {
  int old_bw = board_w, old_bh = board_h;

  query_term_size(term_cols, term_rows);

  int bw = (*term_cols - 4) / 2;
  int bh = *term_rows - 3;
  if (bw < MIN_BOARD_W)
    bw = MIN_BOARD_W;
  if (bh < MIN_BOARD_H)
    bh = MIN_BOARD_H;

  game_set_dimensions(bw, bh);
  render_set_term_size(*term_cols, *term_rows);
  render_clear();

  return (bw != old_bw || bh != old_bh);
}

int main(int argc, char *argv[]) {
  /* ── Initial size: prefer ioctl, fall back to argv from bash ── */
  int term_cols = (argc >= 2) ? atoi(argv[1]) : 80;
  int term_rows = (argc >= 3) ? atoi(argv[2]) : 24;
  query_term_size(&term_cols, &term_rows); /* ioctl wins if available */

  int bw = (term_cols - 4) / 2;
  int bh = term_rows - 3;
  if (bw < MIN_BOARD_W)
    bw = MIN_BOARD_W;
  if (bh < MIN_BOARD_H)
    bh = MIN_BOARD_H;
  game_set_dimensions(bw, bh);
  render_set_term_size(term_cols, term_rows);

  /* ── Cache directory ─────────────────────────────────────── */
  const char *cache = getenv("SNAKE_CACHE");
  if (!cache || cache[0] == '\0')
    cache = "/tmp/snake_cache";
  game_set_cache(cache);

  /* ── SIGWINCH handler ────────────────────────────────────── */
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = on_sigwinch;
  sigaction(SIGWINCH, &sa, NULL);

  input_init();
  render_hide_cursor();
  render_clear();

  Screen screen = SCR_START;
  int menu_sel = 0;
  int high_score = score_load();
  int last_score = 0;
  int running = 1;
  int game_over_drawn = 0;

  GameState game;
  memset(&game, 0, sizeof game);

  struct timespec t0, t1;

  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* ── Handle terminal resize ──────────────────────────── */
    if (resize_pending) {
      resize_pending = 0;
      int board_changed = apply_resize(&term_cols, &term_rows);

      if (screen == SCR_GAME && board_changed) {
        /* Snake coords are invalid for the new board — safe exit */
        game_free(&game);
        screen = SCR_START;
      } else if (screen == SCR_GAMEOVER) {
        game_over_drawn = 0; /* re-draw centred for new size */
      }
      /* SCR_START re-renders naturally each frame */
    }

    /* ── Drain key queue ─────────────────────────────────── */
    int key = -1, k;
    while ((k = input_get_key()) != -1)
      key = k;

    switch (screen) {

    /* ── Start menu ──────────────────────────────────────── */
    case SCR_START:
      if (key == 'w' || key == KEY_UP)
        menu_sel = (menu_sel + MENU_COUNT - 1) % MENU_COUNT;
      if (key == 's' || key == KEY_DOWN)
        menu_sel = (menu_sel + 1) % MENU_COUNT;
      if (key == KEY_ENTER || key == ' ') {
        if (menu_sel == 0) {
          game_init(&game);
          render_clear();
          screen = SCR_GAME;
        } else {
          running = 0;
        }
      }
      if (key == 'q' || key == KEY_ESC)
        running = 0;
      if (running && screen == SCR_START)
        render_start_screen(high_score, menu_sel);
      break;

    /* ── Gameplay ────────────────────────────────────────── */
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
        game_free(&game);
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
        render_clear();
        game_over_drawn = 0;
        screen = SCR_GAMEOVER;
      } else {
        render_game(&game, high_score);
      }
      break;

    /* ── Game over ───────────────────────────────────────── */
    case SCR_GAMEOVER:
      if (!game_over_drawn) {
        render_game_over(last_score, high_score);
        game_over_drawn = 1;
      }
      if (key != -1) {
        render_clear();
        if (key == 'q' || key == KEY_ESC) {
          running = 0;
        } else {
          game_free(&game);
          screen = SCR_START;
        }
      }
      break;
    }

    /* ── Frame-rate limiter ──────────────────────────────── */
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long ns = (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec);
    if (ns < FRAME_NS) {
      struct timespec ts = {0, FRAME_NS - ns};
      nanosleep(&ts, NULL);
    }
  }

  /* ── Teardown ─────────────────────────────────────────────── */
  game_free(&game);
  render_show_cursor();
  render_clear();
  input_restore();

  printf("Thanks for playing Snake! Final best: %d\n", high_score);
  return 0;
}
