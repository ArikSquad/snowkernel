#include "irq.h"
#include "keyboard.h"
#include "kprint.h"
#include "keymap.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ __volatile__("inb %1,%0" : "=a"(r) : "Nd"(port));
    return r;
}

static void kbd_irq(void)
{
    while (1)
    {
        uint8_t status = inb(0x64);
        if (!(status & 1))
            break;
        uint8_t sc = inb(0x60);
        keymap_process_scancode(sc);
    }
}

void irq_kbd_install(void) { irq_install_handler(1, kbd_irq); }
