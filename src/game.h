#ifndef GAME_H
#define GAME_H

#define SNAKE_INIT_LEN 4

/* Snake advances once per MOVE_FRAMES at 60 fps → ~10 moves/sec */
#define MOVE_FRAMES 6

/* Runtime board dimensions — set once via game_set_dimensions() before
   any other game call. All modules include game.h and share these.    */
extern int board_w; /* inner play-area width  (cells)  */
extern int board_h; /* inner play-area height (cells)  */

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;
typedef struct {
  int x, y;
} Point;

typedef struct {
  Point *body; /* ring buffer — heap allocated in game_init() */
  int head;    /* index of head in ring buffer                */
  int length;
  Direction dir;
  Direction next_dir; /* buffered input                              */
} Snake;

typedef struct {
  Snake snake;
  Point apple;
  int score;
  int alive;
  long frame;
} GameState;

/* Call once at startup with the computed play-area dimensions */
void game_set_dimensions(int w, int h);
void game_set_cache(const char *dir);

/* game_init allocates snake.body; game_free releases it     */
void game_init(GameState *g);
void game_free(GameState *g);
void game_update(GameState *g);

int score_load(void);
void score_save(int score);

#endif
