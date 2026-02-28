#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Runtime board globals ───────────────────────────────── */
int board_w = 38; /* defaults; overridden by game_set_dimensions() */
int board_h = 20;

static char cache_dir[512];

void game_set_dimensions(int w, int h) {
  board_w = w;
  board_h = h;
}

void game_set_cache(const char *dir) {
  snprintf(cache_dir, sizeof cache_dir, "%s", dir);
}

/* ── Apple ────────────────────────────────────────────────── */
static void place_apple(GameState *g) {
  int max_snake = board_w * board_h;
  Point p;
  int ok;
  do {
    ok = 1;
    p.x = rand() % board_w;
    p.y = rand() % board_h;
    for (int i = 0; i < g->snake.length; i++) {
      int idx = (g->snake.head - i + max_snake) % max_snake;
      if (g->snake.body[idx].x == p.x && g->snake.body[idx].y == p.y) {
        ok = 0;
        break;
      }
    }
  } while (!ok);
  g->apple = p;
}

/* ── Init / Free ──────────────────────────────────────────── */
void game_init(GameState *g) {
  int max_snake = board_w * board_h;

  /* Free previous allocation if re-starting */
  if (g->snake.body)
    free(g->snake.body);
  memset(g, 0, sizeof *g);

  g->alive = 1;
  g->score = SNAKE_INIT_LEN;

  Snake *s = &g->snake;
  s->body = malloc((size_t)max_snake * sizeof(Point));
  if (!s->body) {
    fputs("snake: out of memory\n", stderr);
    exit(1);
  }
  memset(s->body, 0, (size_t)max_snake * sizeof(Point));

  s->dir = DIR_RIGHT;
  s->next_dir = DIR_RIGHT;
  s->length = SNAKE_INIT_LEN;
  s->head = SNAKE_INIT_LEN - 1;

  int sx = board_w / 2 - SNAKE_INIT_LEN / 2;
  int sy = board_h / 2;
  for (int i = 0; i < SNAKE_INIT_LEN; i++) {
    s->body[i].x = sx + i;
    s->body[i].y = sy;
  }

  srand((unsigned)time(NULL));
  place_apple(g);
}

void game_free(GameState *g) {
  free(g->snake.body);
  g->snake.body = NULL;
}

/* ── Update (called every frame, moves every MOVE_FRAMES) ─── */
void game_update(GameState *g) {
  if (!g->alive)
    return;
  g->frame++;
  if (g->frame % MOVE_FRAMES != 0)
    return;

  int max_snake = board_w * board_h;
  Snake *s = &g->snake;

  /* Apply queued direction — forbid 180° reversal */
  Direction nd = s->next_dir;
  int bad = (s->dir == DIR_UP && nd == DIR_DOWN) ||
            (s->dir == DIR_DOWN && nd == DIR_UP) ||
            (s->dir == DIR_LEFT && nd == DIR_RIGHT) ||
            (s->dir == DIR_RIGHT && nd == DIR_LEFT);
  if (!bad)
    s->dir = nd;

  /* New head position */
  Point h = s->body[s->head];
  switch (s->dir) {
  case DIR_UP:
    h.y--;
    break;
  case DIR_DOWN:
    h.y++;
    break;
  case DIR_LEFT:
    h.x--;
    break;
  case DIR_RIGHT:
    h.x++;
    break;
  }

  /* Wall collision */
  if (h.x < 0 || h.x >= board_w || h.y < 0 || h.y >= board_h) {
    g->alive = 0;
    return;
  }

  /* Self collision (tail will vacate unless growing — skip last segment) */
  int ate = (h.x == g->apple.x && h.y == g->apple.y);
  int chklen = ate ? s->length : s->length - 1;
  for (int i = 0; i < chklen; i++) {
    int idx = (s->head - i + max_snake) % max_snake;
    if (s->body[idx].x == h.x && s->body[idx].y == h.y) {
      g->alive = 0;
      return;
    }
  }

  /* Advance ring buffer */
  s->head = (s->head + 1) % max_snake;
  s->body[s->head] = h;

  if (ate) {
    s->length++;
    g->score = s->length;
    place_apple(g);
  }
}

/* ── High-score persistence ───────────────────────────────── */
int score_load(void) {
  char path[560];
  snprintf(path, sizeof path, "%s/highscore", cache_dir);
  FILE *f = fopen(path, "r");
  if (!f)
    return 0;
  int v = 0;
  int r = fscanf(f, "%d", &v);
  (void)r;
  fclose(f);
  return v;
}

void score_save(int score) {
  char path[560];
  snprintf(path, sizeof path, "%s/highscore", cache_dir);
  FILE *f = fopen(path, "w");
  if (!f)
    return;
  fprintf(f, "%d\n", score);
  fclose(f);
}
