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

static void set_entry64(int vec, void (*handler)(void), uint8_t type_attr)
{
    if (vec < 0 || vec >= IDT_MAX) return;
    uintptr_t addr = (uintptr_t)handler;
#ifdef __SNOW_64__
    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = CODE_SELECTOR;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vec].zero        = 0;
#else
    /* 32-bit layout though we don't really support it anymore */
    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = CODE_SELECTOR;
    idt[vec].zero        = 0;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_high = (addr >> 16) & 0xFFFF;
#endif
}

void idt_set_gate(int vec, void (*handler)(void), uint8_t type_attr)
{
    set_entry64(vec, handler, type_attr);
}

static void default_isr(void)
{
    kprintf("[idt] Unhandled interrupt (continuing)\n");
    /* TODO: make proper handling */
}

void idt_init(void)
{
    for (int i = 0; i < IDT_MAX; i++)
        idt_set_gate(i, default_isr, 0x8E);
    extern void int80_entry64(void);
    idt_set_gate(0x80, (void *)int80_entry64, 0xEE); // user-callable
    /* do a simple #UD handler (vector 6) to report invalid opcode, often
       triggered by unsupported SYSCALL instruction in foreign ELF binaries. */
    extern void invalid_opcode_isr(void);
    idt_set_gate(6, invalid_opcode_isr, 0x8E);
    extern void page_fault_isr(void);
    idt_set_gate(14, page_fault_isr, 0x8E);
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
