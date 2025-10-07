#include "power.h"
#include "kprint.h"
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) { __asm__ __volatile__("outb %0,%1" ::"a"(val), "Nd"(port)); }

void power_shutdown(void)
{
    kprintf("[power] shutdown stub: halting CPU\n");
    // Attempt Bochs/QEMU shutdown via ACPI power-off
    outb(0xB004, 0x2000);
}

void power_reboot(void)
{
    kprintf("[power] reboot stub: triggering reset via keyboard controller\n");
    /* Pulse CPU reset line using 8042 keyboard controller */
    while (1)
    {
        uint8_t status;
        __asm__ __volatile__("inb $0x64,%0" : "=a"(status));
        if (!(status & 0x02))
            break;
    }
    outb(0x64, 0xFE);
    for (;;)
    {
        __asm__ __volatile__("hlt");
    }
}
