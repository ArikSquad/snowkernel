#include "keyboard.h"
#include "../kernel/kprint.h"
#include "keymap.h"
#include "../kernel/tty.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ __volatile__("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

void kbd_set_keymap(int which)
{
    if (which == 1)
        keymap_set_layout(KEYMAP_DVORAK);
    else if (which == 2)
        keymap_set_layout(KEYMAP_FI);
    else
        keymap_set_layout(KEYMAP_US);
}
int kbd_get_keymap(void)
{
    keymap_id_t id = keymap_get_layout();
    return id == KEYMAP_DVORAK ? 1 : (id == KEYMAP_FI ? 2 : 0);
}

int kbd_read_line(char *buf, int max)
{
    int i = 0;
    for (;;)
    {
        int ch = keymap_get_char();
        if (ch < 0)
            continue;
        char c = (char)ch;
        if (c == '\n')
        {
            kputc('\n');
            buf[i] = 0;
            return i;
        }
        if (c == 8)
        { // backspace
            if (i > 0)
            {
                i--;
                kputc('\b');
                kputc(' ');
                kputc('\b');
            }
            continue;
        }
        if (c == '\t')
        {
            for (int t = 0; t < 4 && i < max - 1; ++t)
            {
                buf[i++] = ' ';
                kputc(' ');
            }
            continue;
        }
        if (i < max - 1)
        {
            buf[i++] = c;
            kputc(c);
        }
    }
}

int kbd_getc_nonblock(void) { return keymap_get_char(); }

void kbd_drain_to_tty(void)
{
    int ch;
    while ((ch = keymap_get_char()) >= 0)
    {
        tty_kbd_putc((char)ch);
    }
}
