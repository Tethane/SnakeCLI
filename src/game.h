#ifndef GAME_H
#define GAME_H

#define BOARD_W 38
#define BOARD_H 20
#define MAX_SNAKE (BOARD_W * BOARD_H)
#define SNAKE_INIT_LEN 4

#define MOVE_FRAMES 6

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;
typedef struct {
  int x, y;
} Point;

typedef struct {
  Point body[MAX_SNAKE];
  int head;
  int length;
  Direction dir;
  Direction next_dir;
} Snake;

typedef struct {
  Snake snake;
  Point apple;
  int score;
  int alive;
  long frame;
} GameState;

void game_set_cache(const char *dir);
void game_init(GameState *g);
void game_update(GameState *g);

int score_load(void);
void score_save(int score);

#endif
