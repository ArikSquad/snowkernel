#pragma once
#include <stdint.h>
int ata_read28(uint32_t lba, void *buf);        // returns 0 on success
int ata_write28(uint32_t lba, const void *buf); // returns 0 on success
