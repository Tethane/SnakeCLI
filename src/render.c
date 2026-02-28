#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── ANSI colour macros ──────────────────────────────────── */
#define RST "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"
#define GREEN "\033[32m"
#define CYAN "\033[36m"
#define BRED "\033[91m"
#define BGREEN "\033[92m"
#define BYELLOW "\033[93m"
#define BCYAN "\033[96m"
#define BWHITE "\033[97m"
#define WHITE "\033[37m"

#define HOME "\033[H"
#define CLEAR_SCR "\033[2J\033[H"
#define HIDE_CUR "\033[?25l"
#define SHOW_CUR "\033[?25h"

/* Synchronized Output — tells the terminal to hold rendering until
   BSU_END, eliminating the top-line flicker on every game frame.
   Terminals that don't understand these sequences ignore them.      */
#define BSU_BEGIN "\033[?2026h"
#define BSU_END "\033[?2026l"

/* ── Runtime terminal dimensions ────────────────────────── */
static int term_cols = 80;
static int term_rows = 24;

void render_set_term_size(int cols, int rows) {
  term_cols = cols;
  term_rows = rows;
}

/* ── Output buffer (single fwrite per frame) ─────────────── */
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
static void bnewline(void) { bputc('\n'); }

/* ── Visual width ────────────────────────────────────────────
   Count terminal display columns of a UTF-8 string.
   Each code-point is 1 column (no CJK wide chars in our art).
   We detect code-point boundaries by skipping UTF-8 continuation
   bytes (10xxxxxx), which never start a new character.           */
static int visual_width(const char *s) {
  int w = 0;
  while (*s) {
    unsigned char c = (unsigned char)*s++;
    if ((c & 0xC0) != 0x80)
      w++; /* not a continuation byte */
  }
  return w;
}

/* ── Centring: pads `s` to be centred in `w` display columns ─
   prefix / suffix are ANSI escape strings (zero visual width).  */
static void centre(const char *pre, const char *s, const char *suf, int w) {
  int pad = (w - visual_width(s)) / 2;
  if (pad < 0)
    pad = 0;
  for (int i = 0; i < pad; i++)
    bputc(' ');
  bprint(pre);
  bprint(s);
  bprint(suf);
  bnewline();
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

/* ── Cell grid — heap-allocated lazily ───────────────────── */
typedef enum { CELL_EMPTY, CELL_HEAD, CELL_BODY, CELL_APPLE } Cell;

static Cell *grid = NULL;
static int grid_w = 0, grid_h = 0;

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
    grid[g->snake.body[idx].y * board_w + g->snake.body[idx].x] =
        (i == 0) ? CELL_HEAD : CELL_BODY;
  }
}

/* ── Game screen ─────────────────────────────────────────── */
void render_game(const GameState *g, int high_score) {
  build_grid(g);

  bpos = 0;
  bprint(BSU_BEGIN); /* begin synchronized frame — no partial renders */
  bprint(HOME);

  /* Top border */
  bprint(BOLD CYAN "╔");
  for (int x = 0; x < board_w * 2 + 2; x++)
    bprint("═");
  bprint("╗\n");

  /* Play rows */
  for (int y = 0; y < board_h; y++) {
    bprint(BOLD CYAN "║ " RST);
    for (int x = 0; x < board_w; x++) {
      switch (grid[y * board_w + x]) {
      case CELL_HEAD:
        bprint(BOLD BGREEN "██" RST);
        break;
      case CELL_BODY:
        bprint(GREEN "▓▓" RST);
        break;
      case CELL_APPLE:
        bprint(BOLD BRED "◆◆" RST);
        break;
      default:
        bprint("  ");
        break;
      }
    }
    bprint(BOLD CYAN " ║\n" RST);
  }

  /* Bottom border */
  bprint(BOLD CYAN "╚");
  for (int x = 0; x < board_w * 2 + 2; x++)
    bprint("═");
  bprint("╝\n" RST);

  /* HUD — no trailing \n: emitting \n on the last terminal row scrolls
     the viewport up one line, pushing the top border off screen.      */
  char hud[256];
  snprintf(hud, sizeof hud,
           " " BOLD BYELLOW "SCORE: %-4d" RST "  " DIM BWHITE "BEST: %-4d" RST
           "  " DIM "[ WASD / ↑↓←→ ]  [ Q quit ]" RST,
           g->score, high_score);
  bprint(hud);

  bprint(BSU_END); /* end synchronized frame */
  bflush();
}

/* ── ASCII art ───────────────────────────────────────────── */
static const char *TITLE[] = {
    " ██████╗ ███╗  ██╗ █████╗ ██╗  ██╗███████╗",
    "██╔════╝ ████╗ ██║██╔══██╗██║ ██╔╝██╔════╝",
    " █████╗  ██╔██╗██║███████║█████╔╝ █████╗  ",
    "     ██╗ ██║╚████║██╔══██║██╔═██╗ ██╔══╝  ",
    "██████╔╝ ██║ ╚███║██║  ██║██║  ██╗███████╗",
    "╚═════╝  ╚═╝  ╚══╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝",
};
#define TITLE_LINES 6

static const char *OVER_ART[] = {
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

/* Two-item menu: index 0 = New Game, index 1 = Quit */
static const char *MENU_ITEMS[MENU_COUNT] = {"New Game", "Quit"};

/* ── Vertical padding helper: blank lines to centre content ─ */
static void vpad(int content_lines) {
  int pad = (term_rows - content_lines) / 2;
  if (pad < 1)
    pad = 1;
  for (int i = 0; i < pad; i++)
    bnewline();
}

/* ── Start screen ────────────────────────────────────────── */
void render_start_screen(int high_score, int sel) {
  /* Content line count:
     6 title + 1 gap + 1 tagline + 1 gap + 1 high-score + 2 gap
     + 1 sep + 1 gap + 2 menu (×2 with spacer) + 1 gap + 1 hint = 18 */
  const int CONTENT_H = 18;

  bpos = 0;
  bprint(BSU_BEGIN);
  bprint(HOME);

  int W = term_cols;

  vpad(CONTENT_H);

  /* Title */
  for (int i = 0; i < TITLE_LINES; i++) {
    const char *col = (i == 0 || i == 5) ? BOLD BCYAN : BOLD BGREEN;
    centre(col, TITLE[i], RST, W);
  }
  bnewline();

  /* Tagline */
  centre(DIM WHITE, "~ terminal edition ~", RST, W);
  bnewline();

  /* High score */
  char hs[64];
  snprintf(hs, sizeof hs, "★  All-time best: %d  ★", high_score);
  centre(BOLD BYELLOW, hs, RST, W);
  bnewline();
  bnewline();

  /* Separator */
  centre(DIM CYAN, "───────────────────────────────", RST, W);
  bnewline();

  /* Menu items */
  for (int i = 0; i < MENU_COUNT; i++) {
    /* Build label with cursor indicator */
    char label[64];
    if (i == sel)
      snprintf(label, sizeof label, "▶  %s", MENU_ITEMS[i]);
    else
      snprintf(label, sizeof label, "   %s", MENU_ITEMS[i]);

    const char *col = (i == sel) ? BOLD BGREEN : DIM WHITE;
    centre(col, label, RST, W);
    bnewline();
  }

  /* Nav hint */
  centre(DIM, "W/S  ↑/↓  navigate    Enter  select    Q  quit", RST, W);

  bprint(BSU_END);
  bflush();
}

/* ── Game-over screen ────────────────────────────────────── */
void render_game_over(int score, int high_score) {
  /* Content lines: 12 art + 1 gap + 1 score + 1 best + 2 gap + 1 hint = 18 */
  const int CONTENT_H = 18;

  bpos = 0;
  bprint(BSU_BEGIN);
  bprint(HOME);

  int W = term_cols;

  vpad(CONTENT_H);

  for (int i = 0; i < OVER_LINES; i++) {
    const char *col = (i < 6) ? BOLD BRED : BOLD BWHITE;
    centre(col, OVER_ART[i], RST, W);
  }
  bnewline();

  char sc[64];
  snprintf(sc, sizeof sc, "Score: %d", score);
  centre(BOLD BYELLOW, sc, RST, W);

  if (score >= high_score && score > SNAKE_INIT_LEN)
    centre(BOLD BCYAN, "✦  New High Score!  ✦", RST, W);
  else {
    char best[64];
    snprintf(best, sizeof best, "Best: %d", high_score);
    centre(DIM WHITE, best, RST, W);
  }

  bnewline();
  bnewline();
  centre(DIM, "Press any key to return  ·  Q to quit", RST, W);

  bprint(BSU_END);
  bflush();
}
