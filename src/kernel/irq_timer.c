#include "irq.h"
#include "kprint.h"
#include <stdint.h>

volatile uint64_t ticks = 0;
static void timer_irq(void) { ticks++; }

void irq_timer_install(void) { irq_install_handler(0, timer_irq); }
