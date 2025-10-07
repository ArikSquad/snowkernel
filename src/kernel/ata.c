#include "ata.h"
#include "kprint.h"
#include <stdint.h>

#define ATA_IO_BASE 0x1F0
#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_SECCOUNT0 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07
#define ATA_REG_CONTROL 0x206

#define ATA_CMD_READ_SECT 0x20
#define ATA_CMD_WRITE_SECT 0x30

static inline void outb(uint16_t p, uint8_t v) { __asm__ __volatile__("outb %0,%1" ::"a"(v), "Nd"(p)); }
static inline uint8_t inb(uint16_t p)
{
    uint8_t r;
    __asm__ __volatile__("inb %1,%0" : "=a"(r) : "Nd"(p));
    return r;
}
static inline void rep_insw(uint16_t port, void *addr, int cnt) { __asm__ __volatile__("rep insw" : "=D"(addr), "=c"(cnt) : "d"(port), "0"(addr), "1"(cnt) : "memory"); }
static inline void rep_outsw(uint16_t port, const void *addr, int cnt) { __asm__ __volatile__("rep outsw" ::"d"(port), "S"(addr), "c"(cnt) : "memory"); }

static int ata_wait_drq(void)
{
    for (;;)
    {
        uint8_t s = inb(ATA_IO_BASE + ATA_REG_STATUS);
        if (!(s & 0x80) && (s & 0x08))
            return 0; // not busy & DRQ
        if (s & 0x01)
            return -1; // error
    }
}

int ata_read28(uint32_t lba, void *buf)
{
    if (lba & 0xF0000000)
        return -1;
    outb(ATA_IO_BASE + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_IO_BASE + ATA_REG_SECCOUNT0, 1);
    outb(ATA_IO_BASE + ATA_REG_LBA0, (uint8_t)(lba));
    outb(ATA_IO_BASE + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_IO_BASE + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outb(ATA_IO_BASE + ATA_REG_COMMAND, ATA_CMD_READ_SECT);
    if (ata_wait_drq() != 0)
        return -2;
    rep_insw(ATA_IO_BASE + ATA_REG_DATA, buf, 256);
    return 0;
}

int ata_write28(uint32_t lba, const void *buf)
{
    if (lba & 0xF0000000)
        return -1;
    outb(ATA_IO_BASE + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_IO_BASE + ATA_REG_SECCOUNT0, 1);
    outb(ATA_IO_BASE + ATA_REG_LBA0, (uint8_t)(lba));
    outb(ATA_IO_BASE + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_IO_BASE + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outb(ATA_IO_BASE + ATA_REG_COMMAND, ATA_CMD_WRITE_SECT);
    if (ata_wait_drq() != 0)
        return -2;
    rep_outsw(ATA_IO_BASE + ATA_REG_DATA, buf, 256);
    return 0;
}
