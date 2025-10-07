#include "mouse.h"
#include "kprint.h"
#include "irq.h"
#include <stdint.h>

static int mx = 0, my = 0; /* absolute position */
static int mb = 0;         /* buttons */
static int init_done = 0;

static inline void outb(uint16_t port, uint8_t val) { __asm__ __volatile__("outb %0,%1" ::"a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    __asm__ __volatile__("inb %1,%0" : "=a"(v) : "Nd"(port));
    return v;
}

static void ps2_wait_write(void)
{
    for (int i = 0; i < 100000; i++)
    {
        if (!(inb(0x64) & 0x02))
            return;
    }
}
static void ps2_wait_read(void)
{
    for (int i = 0; i < 100000; i++)
    {
        if (inb(0x64) & 0x01)
            return;
    }
}
static void ps2_write_aux(uint8_t val)
{
    ps2_wait_write();
    outb(0x64, 0xD4);
    ps2_wait_write();
    outb(0x60, val);
}
static uint8_t ps2_read(void)
{
    ps2_wait_read();
    return inb(0x60);
}

static uint8_t mouse_command(uint8_t val)
{
    ps2_write_aux(val);
    return ps2_read();
}

static uint8_t packet[3];
static int packet_index = 0;

static int dbg_counter = 0;
static void mouse_irq(void)
{
    uint8_t status = inb(0x64);
    if (!(status & 0x20))
        return; /* not from mouse */
    uint8_t data = inb(0x60);
    packet[packet_index++] = data;
    if (packet_index == 3)
    {
        packet_index = 0;
        uint8_t b = packet[0];
        int dx = (int)((int8_t)packet[1]);
        int dy = (int)((int8_t)packet[2]);
        mx += dx;
        my -= dy; /* y is inverted */
        if (mx < 0)
            mx = 0;
        if (my < 0)
            my = 0;
        if (mx > 1023)
            mx = 1023;
        if (my > 767)
            my = 767; /* clamp to current mode (assumed) */
        mb = ((b & 1) ? 1 : 0) | ((b & 2) ? 2 : 0) | ((b & 4) ? 4 : 0);
        if (++dbg_counter % 50 == 0)
            kprintf("[mouse] pkt b=%x dx=%d dy=%d mx=%d my=%d btn=%x\n", b, dx, dy, mx, my, mb);
    }
    outb(0x20, 0x20); /* EOI PIC1 */
    outb(0xA0, 0x20); /* EOI PIC2 */
}

void mouse_init(void)
{
    if (init_done)
        return;
    init_done = 1;
    ps2_wait_write();
    outb(0x64, 0xA8);
    ps2_wait_write();
    outb(0x64, 0x20);
    ps2_wait_read();
    uint8_t status = inb(0x60);
    status |= 0x02;
    ps2_wait_write();
    outb(0x64, 0x60);
    ps2_wait_write();
    outb(0x60, status);
    // uint8_t ack = mouse_command(0xFF);
    // if(ack != 0xFA) kprintf("[mouse] reset no ACK (got %x)\n", ack); else { uint8_t self=ps2_read(); (void)self; }
    // ack = mouse_command(0xF6); if(ack!=0xFA) kprintf("[mouse] F6 no ACK (%x)\n", ack);
    uint8_t ack = mouse_command(0xF3);
    if (ack == 0xFA)
    {
        ack = mouse_command(100);
    }

    ack = mouse_command(0xF4);
    if (ack != 0xFA)
        kprintf("[mouse] F4 no ACK (%x)\n", ack);
    irq_install_handler(12, mouse_irq);
    kprintf("[mouse] initialized (IRQ12 handler installed)\n");
}

int mouse_get_position(int *x, int *y)
{
    if (x)
        *x = mx;
    if (y)
        *y = my;
    return 0;
}
int mouse_get_buttons(void) { return mb; }
