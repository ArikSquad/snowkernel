#include "serial.h"

static inline void outb(uint16_t port, uint8_t val) { __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ __volatile__("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

#define COM1 0x3F8

void serial_init(void)
{
    outb(COM1 + 1, 0x00); // Disable all interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1 + 1, 0x00); //                  (hi byte)
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x03); // DTR/RTS set, no loop, OUT2 off
}

static inline int is_transmit_empty() { return inb(COM1 + 5) & 0x20; }
static inline int is_data_ready() { return inb(COM1 + 5) & 0x01; }

int serial_present(void)
{
    outb(COM1 + 7, 0x5A);
    return inb(COM1 + 7) == 0x5A;
}

void serial_putc(char c)
{
    int spins = 100000;
    if (c == '\n')
    {
        while (!is_transmit_empty() && --spins)
            ;
        outb(COM1, '\r');
        spins = 100000;
    }
    while (!is_transmit_empty() && --spins)
        ;
    outb(COM1, (uint8_t)c);
}

int serial_getc_nonblock(void)
{
    if (!is_data_ready())
        return -1;
    return (int)inb(COM1);
}

int serial_read_line(char *buf, int max)
{
    int i = 0;
    for (;;)
    {
        int v = serial_getc_nonblock();
        if (v < 0)
            continue;
        char c = (char)v;
        if (c == '\r')
            c = '\n';
        if (c == '\n')
        {
            serial_putc('\n');
            buf[i] = 0;
            return i;
        }
        if (c == 8 || c == 127)
        {
            if (i > 0)
            {
                i--;
                serial_putc('\b');
                serial_putc(' ');
                serial_putc('\b');
            }
            continue;
        }
        if (i < max - 1)
        {
            buf[i++] = c;
            serial_putc(c);
        }
    }
}
