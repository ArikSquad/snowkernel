#include "irq.h"
#include "idt.h"
#include "pic.h"
#include "kprint.h"

#define IRQ_BASE 0x20
#define MAX_IRQ 16

static void (*irq_handlers[MAX_IRQ])(void);

void irq_common_dispatch(int irq)
{
    if (irq < MAX_IRQ && irq_handlers[irq])
    {
        irq_handlers[irq]();
    }
    else
    {
        static int warned[16];
        if (!warned[irq])
        {
            kprintf("[irq] unhandled IRQ %d\n", irq);
            warned[irq] = 1;
        }
    }
    pic_eoi(irq);
}

void irq_install_handler(int irq, void (*h)(void))
{
    if (irq >= 0 && irq < MAX_IRQ)
        irq_handlers[irq] = h;
}

extern void *irq_stub_ptrs[];   // 64-bit pointer table (if 64-bit)
extern void *irq32_stub_ptrs[]; // 32-bit pointer table (if 32-bit)

void irq_init(void)
{
    void **table;
#ifdef __SNOW_64__
    table = irq_stub_ptrs;
#else
    table = irq32_stub_ptrs;
#endif
    for (int i = 0; i < MAX_IRQ; i++)
    {
        void (*stub)(void) = (void (*)(void))table[i];
        idt_set_gate(IRQ_BASE + i, stub, 0x8E);
    }
    pic_remap(0x20, 0x28);

    for (int i = 0; i < MAX_IRQ; i++)
        pic_set_mask(i);
    pic_clear_mask(0);
    pic_clear_mask(1);
    kprintf("[irq] initialized (timer+keyboard unmasked)\n");
}

void irq_ack(int irq) { pic_eoi(irq); }
