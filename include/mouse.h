#pragma once
void mouse_init(void);
int mouse_get_position(int *x, int *y);
int mouse_get_buttons(void); /* bit0=left bit1=right bit2=middle */
