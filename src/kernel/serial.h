#pragma once
#include <stdint.h>

void serial_init(void);
int serial_present(void);
void serial_putc(char c);
int serial_getc_nonblock(void);           // returns -1 if no char
int serial_read_line(char *buf, int max); // blocking line input
