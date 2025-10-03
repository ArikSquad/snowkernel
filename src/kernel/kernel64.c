#include "kprint.h"
#include "serial.h"
#include "env.h"
#include "shell.h"
#include "../fs/fs.h"
#include "idt.h"
#include "irq.h"
#include "keyboard.h"

void kernel_main64(void)
{
    serial_init();
    kset_color(7, 0);
    kclear();
    kprintf("[kernel64] Bootstage (64-bit)\n");
    idt_init();
    irq_init();

    // install keyboard and timer IRQ handlers
    extern void irq_kbd_install(void);
    irq_kbd_install();
    extern void irq_timer_install(void);
    irq_timer_install();
    extern void keyboard_irq_handler(void);

    if (serial_present())
    {
        kprint_enable_serial();
        kprintf("[conf] Serial console enabled\n");
    }
    kprintf("[fs] fs_init() starting...\n");
    fs_init();
    kprintf("[fs] fs_init() done\n");
    fs_mkdir(fs_root(), "home");
    env_init();
    kprintf("[env] initialized PATH=%s\n", env_get("PATH"));
    kprintf("[shell] Ready. Type 'help' for commands (64-bit)\n\n");
    idt_enable();
    shell_run();
}
