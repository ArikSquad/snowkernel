#pragma once
#include <stdint.h>
int kbd_read_line(char *buf, int max);
int kbd_getc_nonblock(void);    // returns -1 if no char
void kbd_set_keymap(int which); // 0=us,1=dvorak
int kbd_get_keymap(void);
