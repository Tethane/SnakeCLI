#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char cache_dir[512];

void game_set_cache(const char *dir) {
  snprintf(cache_dir, sizeof(cache_dir), "%s", dir);
}

static void place_apple(GameState *g) {
  Point p;
  int ok;
  do {
    ok = 1;
    p.x = rand() % BOARD_W;
    p.y = rand() % BOARD_H;
    for (int i = 0; i < g->snake.length; i++) {
      int idx = (g->snake.head - i + MAX_SNAKE) % MAX_SNAKE;
      if (g->snake.body[idx].x == p.x && g->snake.body[idx].y == p.y) {
        ok = 0;
        break;
      }
    }
  } while (!ok);
  g->apple = p;
}

void game_init(GameState *g) {
  memset(g, 0, sizeof *g);
  g->alive = 1;
  g->score = SNAKE_INIT_LEN;

  Snake *s = &g->snake;
  s->dir = DIR_RIGHT;
  s->next_dir = DIR_RIGHT;
  s->length = SNAKE_INIT_LEN;
  s->head = SNAKE_INIT_LEN - 1;

  int sx = BOARD_W / 2 - SNAKE_INIT_LEN / 2;
  int sy = BOARD_H / 2;

  for (int i = 0; i < SNAKE_INIT_LEN; i++) {
    s->body[i].x = sx + i;
    s->body[i].y = sy;
  }

  srand((unsigned)time(NULL));
  place_apple(g);
}

void game_update(GameState *g) {
  if (!g->alive)
    return;
  g->frame++;
  if (g->frame % MOVE_FRAMES != 0)
    return;

  Snake *s = &g->snake;

  Direction nd = s->next_dir;
  int bad = (s->dir == DIR_UP && nd == DIR_DOWN) ||
            (s->dir == DIR_DOWN && nd == DIR_UP) ||
            (s->dir == DIR_LEFT && nd == DIR_RIGHT) ||
            (s->dir == DIR_RIGHT && nd == DIR_LEFT);
  if (!bad)
    s->dir = nd;

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

  if (h.x < 0 || h.x >= BOARD_W || h.y < 0 || h.y >= BOARD_H) {
    g->alive = 0;
    return;
  }

  int ate = (h.x == g->apple.x && h.y == g->apple.y);
  int chklen = ate ? s->length : s->length - 1;
  for (int i = 0; i < chklen; i++) {
    int idx = (s->head - i + MAX_SNAKE) % MAX_SNAKE;
    if (s->body[idx].x == h.x && s->body[idx].y == h.y) {
      g->alive = 0;
      return;
    }
  }

  s->head = (s->head + 1) % MAX_SNAKE;
  s->body[s->head] = h;

  if (ate) {
    s->length++;
    g->score = s->length;
    place_apple(g);
  }
}

int score_load(void) {
  char path[560];
  snprintf(path, sizeof path, "%s/highscore", cache_dir);
  FILE *f = fopen(path, "r");
  if (!f)
    return 0;
  int v = 0;
  fscanf(f, "%d", &v);
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
