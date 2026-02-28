#include "input.h"
#include <termios.h>
#include <unistd.h>

static struct termios orig;

void input_init(void) {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &orig);
  raw = orig;
  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void input_restore(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }

int input_get_key(void) {
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) != 1)
    return -1;

  if (c == 27) {
    unsigned char s[2];
    /* Arrow keys: ESC [ A/B/C/D */
    if (read(STDIN_FILENO, &s[0], 1) == 1 &&
        read(STDIN_FILENO, &s[1], 1) == 1 && s[0] == '[') {
      switch (s[1]) {
      case 'A':
        return KEY_UP;
      case 'B':
        return KEY_DOWN;
      case 'C':
        return KEY_RIGHT;
      case 'D':
        return KEY_LEFT;
      }
    }
    return KEY_ESC;
  }
  /* Normalise carriage return / newline */
  if (c == '\r' || c == '\n')
    return KEY_ENTER;
  return (int)c;
}
