#pragma once
#include <stdint.h>

#ifdef __SNOW_64__
typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} idt_entry_t;
#else
typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} idt_entry_t;
#endif

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uintptr_t base;
} idt_ptr_t;

void idt_init(void);
void idt_set_gate(int vec, void (*handler)(void), uint8_t type_attr);
void idt_enable(void); // call after PIC remap / IRQ handlers installed

// debug
void idt_dump(void);
