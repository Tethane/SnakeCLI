#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── ANSI helpers ─────────────────────────────────────────── */
#define CLR_RESET "\033[0m"
#define CLR_BOLD "\033[1m"
#define CLR_DIM "\033[2m"
#define CLR_GREEN "\033[32m"
#define CLR_CYAN "\033[36m"
#define CLR_BRED "\033[91m"
#define CLR_BGREEN "\033[92m"
#define CLR_BYELLOW "\033[93m"
#define CLR_BCYAN "\033[96m"
#define CLR_BWHITE "\033[97m"
#define CLR_WHITE "\033[37m"

#define HOME "\033[H"
#define CLEAR_SCR "\033[2J\033[H"
#define HIDE_CUR "\033[?25l"
#define SHOW_CUR "\033[?25h"

/* Runtime display width: border + padding + cells + padding + border */
#define DISP_W (board_w * 2 + 4)

/* ── Output buffer ───────────────────────────────────────── */
#define BUF_SZ (1024 * 128)
static char buf[BUF_SZ];
static int bpos;

static void bflush(void) {
  fwrite(buf, 1, bpos, stdout);
  fflush(stdout);
  bpos = 0;
}
static void bprint(const char *s) {
  int l = (int)strlen(s);
  if (bpos + l >= BUF_SZ)
    bflush();
  memcpy(buf + bpos, s, (size_t)l);
  bpos += l;
}
static void bputc(char c) {
  if (bpos + 1 >= BUF_SZ)
    bflush();
  buf[bpos++] = c;
}

/* ── Cursor / clear ──────────────────────────────────────── */
void render_hide_cursor(void) {
  fputs(HIDE_CUR, stdout);
  fflush(stdout);
}
void render_show_cursor(void) {
  fputs(SHOW_CUR, stdout);
  fflush(stdout);
}
void render_clear(void) {
  fputs(CLEAR_SCR, stdout);
  fflush(stdout);
}

/* ── Cell grid — heap-allocated lazily at first use ─────── */
typedef enum { CELL_EMPTY, CELL_HEAD, CELL_BODY, CELL_APPLE } Cell;

static Cell *grid = NULL;
static int grid_w = 0;
static int grid_h = 0;

static void grid_ensure(void) {
  if (grid && grid_w == board_w && grid_h == board_h)
    return;
  free(grid);
  grid = malloc((size_t)(board_w * board_h) * sizeof(Cell));
  grid_w = board_w;
  grid_h = board_h;
}

static void build_grid(const GameState *g) {
  grid_ensure();
  int sz = board_w * board_h;
  memset(grid, CELL_EMPTY, (size_t)sz * sizeof(Cell));

  grid[g->apple.y * board_w + g->apple.x] = CELL_APPLE;

  for (int i = g->snake.length - 1; i >= 0; i--) {
    int idx = (g->snake.head - i + sz) % sz;
    int x = g->snake.body[idx].x;
    int y = g->snake.body[idx].y;
    grid[y * board_w + x] = (i == 0) ? CELL_HEAD : CELL_BODY;
  }
}

/* ── Game screen ─────────────────────────────────────────── */
void render_game(const GameState *g, int high_score) {
  build_grid(g);

  bpos = 0;
  bprint(HOME);

  /* Top border */
  bprint(CLR_BOLD CLR_CYAN "╔");
  for (int x = 0; x < board_w * 2 + 2; x++)
    bprint("═");
  bprint("╗\n");

  /* Rows */
  for (int y = 0; y < board_h; y++) {
    bprint(CLR_BOLD CLR_CYAN "║ " CLR_RESET);
    for (int x = 0; x < board_w; x++) {
      switch (grid[y * board_w + x]) {
      case CELL_HEAD:
        bprint(CLR_BOLD CLR_BGREEN "██" CLR_RESET);
        break;
      case CELL_BODY:
        bprint(CLR_GREEN "▓▓" CLR_RESET);
        break;
      case CELL_APPLE:
        bprint(CLR_BOLD CLR_BRED "◆◆" CLR_RESET);
        break;
      default:
        bprint("  ");
        break;
      }
    }
    bprint(CLR_BOLD CLR_CYAN " ║\n" CLR_RESET);
  }

  /* Bottom border */
  bprint(CLR_BOLD CLR_CYAN "╚");
  for (int x = 0; x < board_w * 2 + 2; x++)
    bprint("═");
  bprint("╝\n" CLR_RESET);

  /* HUD */
  char hud[256];
  snprintf(hud, sizeof hud,
           " " CLR_BOLD CLR_BYELLOW "SCORE: %-4d" CLR_RESET
           "  " CLR_DIM CLR_BWHITE "BEST: %-4d" CLR_RESET "  " CLR_DIM
           "[ WASD / ↑↓←→ ]  [ Q quit ]\n" CLR_RESET,
           g->score, high_score);
  bprint(hud);

  bflush();
}

/* ── Centring helper ─────────────────────────────────────── */
static void centre_raw(const char *pre, const char *s, const char *suf, int w) {
  int pad = (w - (int)strlen(s)) / 2;
  for (int i = 0; i < pad; i++)
    bputc(' ');
  bprint(pre);
  bprint(s);
  bprint(suf);
  bputc('\n');
}

/* ── ASCII title art ─────────────────────────────────────── */
static const char *TITLE[] = {
    " ██████╗ ███╗  ██╗ █████╗ ██╗  ██╗███████╗",
    "██╔════╝ ████╗ ██║██╔══██╗██║ ██╔╝██╔════╝",
    " █████╗  ██╔██╗██║███████║█████╔╝ █████╗  ",
    "     ██╗ ██║╚████║██╔══██║██╔═██╗ ██╔══╝  ",
    "██████╔╝ ██║ ╚███║██║  ██║██║  ██╗███████╗",
    "╚═════╝  ╚═╝  ╚══╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝",
};
#define TITLE_LINES 6

static const char *MENU_LABELS[] = {"▶  New Game", "   High Score", "   Quit"};
#define MENU_COUNT 3

void render_start_screen(int high_score, int sel) {
  bpos = 0;
  bprint(HOME);

  int W = DISP_W;
  bputc('\n');

  for (int i = 0; i < TITLE_LINES; i++) {
    const char *col =
        (i == 0 || i == 5) ? CLR_BOLD CLR_BCYAN : CLR_BOLD CLR_BGREEN;
    centre_raw(col, TITLE[i], CLR_RESET, W);
  }
  bputc('\n');

  centre_raw(CLR_DIM CLR_WHITE, "~ terminal edition ~", CLR_RESET, W);
  bputc('\n');

  char hs[64];
  snprintf(hs, sizeof hs, "★  All-time best: %d  ★", high_score);
  centre_raw(CLR_BOLD CLR_BYELLOW, hs, CLR_RESET, W);
  bputc('\n');
  bputc('\n');

  centre_raw(CLR_DIM CLR_CYAN, "─────────────────────────────", CLR_RESET, W);
  bputc('\n');

  for (int i = 0; i < MENU_COUNT; i++) {
    const char *col = (i == sel) ? CLR_BOLD CLR_BGREEN : CLR_DIM CLR_WHITE;
    centre_raw(col, MENU_LABELS[i], CLR_RESET, W);
    bputc('\n');
  }

  bputc('\n');
  centre_raw(CLR_DIM, "W/S or ↑/↓ to navigate · Enter to select", CLR_RESET, W);
  bputc('\n');

  bflush();
}

/* ── Game-over screen ────────────────────────────────────── */
static const char *OVER[] = {
    "  ██████╗  █████╗ ███╗   ███╗███████╗",
    " ██╔════╝ ██╔══██╗████╗ ████║██╔════╝",
    " ██║  ███╗███████║██╔████╔██║█████╗  ",
    " ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝  ",
    " ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗",
    "  ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝",
    "  ██████╗ ██╗   ██╗███████╗██████╗   ",
    " ██╔═══██╗██║   ██║██╔════╝██╔══██╗  ",
    " ██║   ██║██║   ██║█████╗  ██████╔╝  ",
    " ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗  ",
    " ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║  ",
    "  ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝  ",
};
#define OVER_LINES 12

void render_game_over(int score, int high_score) {
  bpos = 0;
  bprint(HOME);

  int W = DISP_W;
  bputc('\n');
  bputc('\n');

  for (int i = 0; i < OVER_LINES; i++) {
    const char *col = (i < 6) ? CLR_BOLD CLR_BRED : CLR_BOLD CLR_BWHITE;
    centre_raw(col, OVER[i], CLR_RESET, W);
  }
  bputc('\n');

  char sc[64];
  snprintf(sc, sizeof sc, "Score: %d", score);
  centre_raw(CLR_BOLD CLR_BYELLOW, sc, CLR_RESET, W);
  bputc('\n');

  if (score >= high_score && score > SNAKE_INIT_LEN)
    centre_raw(CLR_BOLD CLR_BCYAN, "✦  New High Score!  ✦", CLR_RESET, W);
  else {
    char best[64];
    snprintf(best, sizeof best, "Best: %d", high_score);
    centre_raw(CLR_DIM CLR_WHITE, best, CLR_RESET, W);
  }

  bputc('\n');
  bputc('\n');
  centre_raw(CLR_DIM, "Press any key to return to menu  ·  Q to quit",
             CLR_RESET, W);
  bputc('\n');

  bflush();
}
