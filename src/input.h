#ifndef INPUT_H
#define INPUT_H

#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_LEFT 3
#define KEY_RIGHT 4
#define KEY_ESC 27
#define KEY_ENTER 13

void input_init(void);
void input_restore(void);
int input_get_key(void); /* returns key code or -1 if no input */

#endif
