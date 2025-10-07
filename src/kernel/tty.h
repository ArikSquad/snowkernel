#pragma once
#include <stddef.h>
#include <stdint.h>

#define TTY_MAX 4
#define TTY_BUF 1024

typedef struct tty
{
    int index;
    char inbuf[TTY_BUF];
    volatile uint16_t in_head; // write position (producer: IRQ)
    volatile uint16_t in_tail; // read position (consumer: sys_read)
    int foreground;            // 1 if active tty for keyboard input
} tty_t;

void tty_init(void);
struct tty *tty_get(int idx);
struct tty *tty_active(void);
void tty_set_active(int idx);

void tty_kbd_putc(char c);
int tty_read(tty_t *t, char *buf, size_t count);
int tty_write(tty_t *t, const char *buf, size_t count);
void tty_register_fs(void);
