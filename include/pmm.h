#pragma once
#include <stddef.h>
#include <stdint.h>

void pmm_init(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *p);
