#include "idt.h"
#include "kprint.h"

#define IDT_MAX 256
static idt_entry_t idt[IDT_MAX];
static idt_ptr_t idtp;

#ifdef __SNOW_64__
#define CODE_SELECTOR 0x08
#else
#define CODE_SELECTOR 0x08
#endif

#ifdef __SNOW_64__
static void set_entry64(int vec, void (*handler)(void), uint8_t type_attr)
{
    uintptr_t addr = (uintptr_t)handler;
    idt[vec].offset_low = addr & 0xFFFF;
    idt[vec].selector = CODE_SELECTOR;
    idt[vec].ist = 0;
    idt[vec].type_attr = type_attr; // present=1, DPL, type=0xE interrupt gate
    idt[vec].offset_mid = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vec].zero = 0;
}
#else
static void set_entry32(int vec, void (*handler)(void), uint8_t type_attr)
{
    uintptr_t addr = (uintptr_t)handler;
    idt[vec].offset_low = addr & 0xFFFF;
    idt[vec].selector = CODE_SELECTOR;
    idt[vec].zero = 0;
    idt[vec].type_attr = type_attr;
    idt[vec].offset_high = (addr >> 16) & 0xFFFF;
}
#endif

void idt_set_gate(int vec, void (*handler)(void), uint8_t type_attr)
{
    if (vec < 0 || vec >= IDT_MAX)
        return;
#ifdef __SNOW_64__
    set_entry64(vec, handler, type_attr);
#else
    set_entry32(vec, handler, type_attr);
#endif
}

static void default_isr(void)
{
    kprintf("[idt] Unhandled interrupt\n");
    for (;;)
    {
        __asm__ __volatile__("hlt");
    }
}

extern void int80_entry(void);   // 32-bit syscall entry
extern void int80_entry64(void); // will be provided for x86_64

void idt_init(void)
{
    for (int i = 0; i < IDT_MAX; i++)
        idt_set_gate(i, default_isr, 0x8E);
#ifndef __SNOW_64__
    idt_set_gate(0x80, (void *)int80_entry, 0xEE); // DPL=3 (0xE | 0x60) + present
#else
    idt_set_gate(0x80, (void *)int80_entry64, 0xEE); // user-callable
#endif
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uintptr_t)&idt[0];
    __asm__ __volatile__("lidt (%0)" ::"r"(&idtp));
    kprintf("[idt] loaded IDT base=0x%x limit=%u\n", (unsigned)idtp.base, idtp.limit);
}

void idt_enable(void) { __asm__ __volatile__("sti"); }

void idt_dump(void)
{
    for (int i = 0; i < 16; i++)
    {
        kprintf("[idt] vec %d entry @%x\n", i, (unsigned)(uintptr_t)(&idt[i]));
    }
}
