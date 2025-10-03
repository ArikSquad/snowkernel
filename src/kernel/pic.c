#include "pic.h"
#include "kprint.h"

static inline void outb(uint16_t port, uint8_t val) { __asm__ __volatile__("outb %0,%1" ::"a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ __volatile__("inb %1,%0" : "=a"(r) : "Nd"(port));
    return r;
}

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

void pic_remap(int offset1, int offset2)
{
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
    kprintf("[pic] remapped to %d %d\n", offset1, offset2);
}

static inline uint16_t pic_get_masks() { return (uint16_t)(inb(PIC2_DATA) << 8) | inb(PIC1_DATA); }
static inline void pic_set_masks(uint16_t m)
{
    outb(PIC1_DATA, (uint8_t)(m & 0xFF));
    outb(PIC2_DATA, (uint8_t)(m >> 8));
}

void pic_set_mask(uint8_t irq_line)
{
    uint16_t m = pic_get_masks();
    m |= (1u << irq_line);
    pic_set_masks(m);
}
void pic_clear_mask(uint8_t irq_line)
{
    uint16_t m = pic_get_masks();
    m &= ~(1u << irq_line);
    pic_set_masks(m);
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}
