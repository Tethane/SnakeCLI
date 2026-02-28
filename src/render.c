#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── ANSI helpers ─────────────────────────────────────────── */
#define CLR_RESET "\033[0m"
#define CLR_BOLD "\033[1m"
#define CLR_DIM "\033[2m"
#define CLR_BLACK "\033[30m"
#define CLR_RED "\033[31m"
#define CLR_GREEN "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_CYAN "\033[36m"
#define CLR_WHITE "\033[37m"
#define CLR_BRED "\033[91m"
#define CLR_BGREEN "\033[92m"
#define CLR_BYELLOW "\033[93m"
#define CLR_BCYAN "\033[96m"
#define CLR_BWHITE "\033[97m"

#define HOME "\033[H"
#define CLEAR_SCR "\033[2J\033[H"
#define HIDE_CUR "\033[?25l"
#define SHOW_CUR "\033[?25h"

/* Each board cell renders as 2 chars wide → total display width:
   2 border cols + BOARD_W*2 + 2 border cols = BOARD_W*2 + 4          */
#define DISP_W (BOARD_W * 2 + 4)

/* Large output buffer written in a single fwrite per frame            */
#define BUF_SZ (1024 * 64)
static char buf[BUF_SZ];
static int bpos;

static void bflush(void) {
  fwrite(buf, 1, bpos, stdout);
  fflush(stdout);
  bpos = 0;
}
static void bprint(const char *s) {
  int l = strlen(s);
  if (bpos + l >= BUF_SZ)
    bflush();
  memcpy(buf + bpos, s, l);
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

/* ── Cell grid helpers ───────────────────────────────────── */
typedef enum { CELL_EMPTY, CELL_WALL, CELL_HEAD, CELL_BODY, CELL_APPLE } Cell;

static Cell grid[BOARD_H][BOARD_W];

static void build_grid(const GameState *g) {
  memset(grid, CELL_EMPTY, sizeof grid);

  /* Apple */
  grid[g->apple.y][g->apple.x] = CELL_APPLE;

  /* Snake body (tail → head) */
  for (int i = g->snake.length - 1; i >= 0; i--) {
    int idx = (g->snake.head - i + MAX_SNAKE) % MAX_SNAKE;
    int x = g->snake.body[idx].x;
    int y = g->snake.body[idx].y;
    grid[y][x] = (i == 0) ? CELL_HEAD : CELL_BODY;
  }
}

/* ── Game screen ─────────────────────────────────────────── */
void render_game(const GameState *g, int high_score) {
  build_grid(g);

  bpos = 0;
  bprint(HOME);

  /* ── Top border ── */
  bprint(CLR_BOLD CLR_CYAN);
  bprint("╔");
  for (int x = 0; x < BOARD_W * 2 + 2; x++)
    bprint("═");
  bprint("╗\n");

  /* ── Rows ── */
  for (int y = 0; y < BOARD_H; y++) {
    bprint(CLR_BOLD CLR_CYAN "║ " CLR_RESET);
    for (int x = 0; x < BOARD_W; x++) {
      switch (grid[y][x]) {
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

  /* ── Bottom border ── */
  bprint(CLR_BOLD CLR_CYAN "╚");
  for (int x = 0; x < BOARD_W * 2 + 2; x++)
    bprint("═");
  bprint("╝\n" CLR_RESET);

  /* ── HUD ── */
  char hud[128];
  snprintf(hud, sizeof hud,
           " " CLR_BOLD CLR_BYELLOW "SCORE: %-4d" CLR_RESET
           "  " CLR_DIM CLR_BWHITE "BEST: %-4d" CLR_RESET "  " CLR_DIM
           "[ WASD / ↑↓←→ ]  [ Q quit ]\n" CLR_RESET,
           g->score, high_score);
  bprint(hud);

  bflush();
}

/* ── ASCII art title ─────────────────────────────────────── */
static const char *TITLE[] = {
    " ██████╗ ███╗  ██╗ █████╗ ██╗  ██╗███████╗",
    "██╔════╝ ████╗ ██║██╔══██╗██║ ██╔╝██╔════╝",
    " █████╗  ██╔██╗██║███████║█████╔╝ █████╗  ",
    "     ██╗ ██║╚████║██╔══██║██╔═██╗ ██╔══╝  ",
    "██████╔╝ ██║ ╚███║██║  ██║██║  ██╗███████╗",
    "╚═════╝  ╚═╝  ╚══╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝",
};
#define TITLE_LINES 6
#define TITLE_W 44 /* visual width of title art */

/* Centre a string in a field of width `w` (pad with spaces) — kept for
   potential future use but not currently referenced from exported API. */
static void __attribute__((unused)) centre(const char *s, int w) {
  int len = strlen(s);
  int pad = (w - len) / 2;
  for (int i = 0; i < pad; i++)
    bputc(' ');
  bprint(s);
  bputc('\n');
}

static void centre_raw(const char *prefix, const char *s, const char *suffix,
                       int w) {
  /* prefix/suffix are ANSI codes that don't count toward visual width */
  int len = strlen(s);
  int pad = (w - len) / 2;
  for (int i = 0; i < pad; i++)
    bputc(' ');
  bprint(prefix);
  bprint(s);
  bprint(suffix);
  bputc('\n');
}

/* ── Start screen ────────────────────────────────────────── */
static const char *MENU_LABELS[] = {"▶  New Game", "   High Score", "   Quit"};
static const int MENU_COUNT = 3;

void render_start_screen(int high_score, int sel) {
  bpos = 0;
  bprint(HOME);

  int W = DISP_W; /* ~80 cols */

  /* blank buffer line */
  bputc('\n');

  /* Title art */
  for (int i = 0; i < TITLE_LINES; i++) {
    if (i == 0 || i == 5)
      centre_raw(CLR_BOLD CLR_BCYAN, TITLE[i], CLR_RESET, W);
    else
      centre_raw(CLR_BOLD CLR_BGREEN, TITLE[i], CLR_RESET, W);
  }

  bputc('\n');

  /* Sub-tagline */
  centre_raw(CLR_DIM CLR_WHITE, "~ terminal edition ~", CLR_RESET, W);
  bputc('\n');

  /* High score badge */
  char hs[64];
  snprintf(hs, sizeof hs, "★  All-time best: %d  ★", high_score);
  centre_raw(CLR_BOLD CLR_BYELLOW, hs, CLR_RESET, W);
  bputc('\n');
  bputc('\n');

  /* Separator */
  centre_raw(CLR_DIM CLR_CYAN, "─────────────────────────────", CLR_RESET, W);
  bputc('\n');

  /* Menu items */
  for (int i = 0; i < MENU_COUNT; i++) {
    if (i == sel) {
      centre_raw(CLR_BOLD CLR_BGREEN, MENU_LABELS[i], CLR_RESET, W);
    } else {
      centre_raw(CLR_DIM CLR_WHITE, MENU_LABELS[i], CLR_RESET, W);
    }
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
    if (i < 6)
      centre_raw(CLR_BOLD CLR_BRED, OVER[i], CLR_RESET, W);
    else
      centre_raw(CLR_BOLD CLR_BWHITE, OVER[i], CLR_RESET, W);
  }

  bputc('\n');

  char sc[64];
  snprintf(sc, sizeof sc, "Score: %d", score);
  centre_raw(CLR_BOLD CLR_BYELLOW, sc, CLR_RESET, W);
  bputc('\n');

  if (score >= high_score && score > SNAKE_INIT_LEN) {
    centre_raw(CLR_BOLD CLR_BCYAN, "✦  New High Score!  ✦", CLR_RESET, W);
  } else {
    char hs[64];
    snprintf(hs, sizeof hs, "Best: %d", high_score);
    centre_raw(CLR_DIM CLR_WHITE, hs, CLR_RESET, W);
  }

  bputc('\n');
  bputc('\n');
  centre_raw(CLR_DIM, "Press any key to return to menu  ·  Q to quit",
             CLR_RESET, W);
  bputc('\n');

  bflush();
}
