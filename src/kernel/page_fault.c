#include "kprint.h"
#include <stdint.h>

void page_fault_handler_c(void *frame)
{
    (void)frame;
    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    kprintf("[pf] Page fault at address=%x (CR2) -- halting\n", (unsigned)cr2);
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}