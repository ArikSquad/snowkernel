#pragma once
#include <stdint.h>

void irq_init(void);
void irq_install_handler(int irq, void (*h)(void));
void irq_ack(int irq);
