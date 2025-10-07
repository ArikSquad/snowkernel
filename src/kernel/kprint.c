#include "kprint.h"
#include "string.h"
#include "serial.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t *)0xB8000)

static uint16_t *const vga = VGA_MEMORY;
static size_t cx = 0, cy = 0;
static uint8_t color = 0x07;
static kputc_fn sink = 0;

static inline uint16_t make_cell(char ch) { return ((uint16_t)color << 8) | (uint8_t)ch; }

static void scroll()
{
    if (cy < VGA_HEIGHT)
        return;

    kmemcpy(vga, vga + VGA_WIDTH, (VGA_HEIGHT - 1) * VGA_WIDTH * sizeof(uint16_t));
    for (size_t x = 0; x < VGA_WIDTH; ++x)
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_cell(' ');
    cy = VGA_HEIGHT - 1;
}

static void newline()
{
    cx = 0;
    cy++;
    scroll();
}

static inline void update_hw_cursor()
{
    uint16_t pos = (uint16_t)(cy * VGA_WIDTH + cx);
    // VGA cursor ports 0x3D4 (index) and 0x3D5 (data)
    // Low byte index 0x0F, high byte index 0x0E
    __asm__ __volatile__("outb %0, %1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ __volatile__("outb %0, %1" : : "a"((uint8_t)(pos & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ __volatile__("outb %0, %1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ __volatile__("outb %0, %1" : : "a"((uint8_t)(pos >> 8)), "Nd"((uint16_t)0x3D5));
}

void kset_color(uint8_t fg, uint8_t bg) { color = (bg << 4) | (fg & 0x0F); }

static void vga_putc(char c)
{
    if (c == '\n')
    {
        newline();
        update_hw_cursor();
        return;
    }
    if (c == '\r')
    {
        cx = 0;
        return;
    }
    if (c == '\b')
    {
        if (cx > 0)
        {
            cx--;
            vga[cy * VGA_WIDTH + cx] = make_cell(' ');
        }
        update_hw_cursor();
        return;
    }
    if (c == '\t')
    {
        size_t n = 4 - (cx & 3);
        while (n--)
            vga_putc(' ');
        update_hw_cursor();
        return;
    }
    vga[cy * VGA_WIDTH + cx] = make_cell(c);
    if (++cx >= VGA_WIDTH)
    {
        newline();
    }
    update_hw_cursor();
}

static void vga_serial_putc(char c)
{
    vga_putc(c); // VGA first
    serial_putc(c);
}

void kprint_enable_serial(void) { kprint_set_sink(vga_serial_putc); }

void kputc(char c)
{
    if (sink)
        sink(c);
    else
        vga_putc(c);
}
void kputs(const char *s)
{
    while (*s)
        kputc(*s++);
}

static void kvprintf(const char *fmt, va_list ap)
{
    for (const char *p = fmt; *p; ++p)
    {
        if (*p != '%')
        {
            kputc(*p);
            continue;
        }
        ++p;
        switch (*p)
        {
        case 's':
        {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            kputs(s);
            break;
        }
        case 'c':
        {
            char c = (char)va_arg(ap, int);
            kputc(c);
            break;
        }
        case 'd':
        {
            int v = va_arg(ap, int);
            if (v < 0)
            {
                kputc('-');
                v = -v;
            }
            char buf[16];
            int i = 0;
            do
            {
                buf[i++] = '0' + (v % 10);
                v /= 10;
            } while (v && i < 15);
            while (i--)
                kputc(buf[i]);
            break;
        }
        case 'u':
        {
            unsigned v = va_arg(ap, unsigned);
            char buf[16];
            int i = 0;
            do
            {
                buf[i++] = '0' + (v % 10);
                v /= 10;
            } while (v && i < 15);
            while (i--)
                kputc(buf[i]);
            break;
        }
        case 'x':
        {
            unsigned v = va_arg(ap, unsigned);
            char *hex = "0123456789abcdef";
            char buf[16];
            int i = 0;
            do
            {
                buf[i++] = hex[v & 15];
                v >>= 4;
            } while (v && i < 15);
            if (i == 0)
                kputc('0');
            else
                while (i--)
                    kputc(buf[i]);
            break;
        }
        case 'p':
        {
            unsigned long long v = (unsigned long long)va_arg(ap, void *);
            const char *hex = "0123456789abcdef";
            kputc('0'); kputc('x');
            /* print full 16 hex digits */
            for (int shift = 60; shift >= 0; shift -= 4) {
                unsigned d = (v >> shift) & 0xF;
                kputc(hex[d]);
            }
            break;
        }
        case '%':
            kputc('%');
            break;
        default:
            kputc('%');
            kputc(*p);
            break;
        }
    }
}

void kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}

/* Internal helper: write a single char into a bounded buffer */
static void buf_putc(char **bufp, size_t *rem, char c)
{
    if (*rem > 1) {
        **bufp = c;
        (*bufp)++;
        (*rem)--;
    }
}

static void kvsnprintf(char **bufp, size_t *rem, const char *fmt, va_list ap)
{
    for (const char *p = fmt; *p; ++p)
    {
        if (*p != '%')
        {
            buf_putc(bufp, rem, *p);
            continue;
        }
        ++p;
        switch (*p)
        {
        case 's':
        {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            while (*s) buf_putc(bufp, rem, *s++);
            break;
        }
        case 'c':
        {
            char c = (char)va_arg(ap, int);
            buf_putc(bufp, rem, c);
            break;
        }
        case 'd':
        {
            int v = va_arg(ap, int);
            if (v < 0)
            {
                buf_putc(bufp, rem, '-');
                v = -v;
            }
            char buf[16]; int i = 0;
            do { buf[i++] = '0' + (v % 10); v /= 10; } while (v && i < 15);
            while (i--) buf_putc(bufp, rem, buf[i]);
            break;
        }
        case 'u':
        {
            unsigned v = va_arg(ap, unsigned);
            char buf[16]; int i = 0;
            do { buf[i++] = '0' + (v % 10); v /= 10; } while (v && i < 15);
            while (i--) buf_putc(bufp, rem, buf[i]);
            break;
        }
        case 'x':
        {
            unsigned v = va_arg(ap, unsigned);
            const char *hex = "0123456789abcdef";
            char buf[16]; int i = 0;
            do { buf[i++] = hex[v & 15]; v >>= 4; } while (v && i < 15);
            if (i == 0) buf_putc(bufp, rem, '0'); else while (i--) buf_putc(bufp, rem, buf[i]);
            break;
        }
        case 'p':
        {
            unsigned long long v = (unsigned long long)va_arg(ap, void *);
            const char *hex = "0123456789abcdef";
            buf_putc(bufp, rem, '0'); buf_putc(bufp, rem, 'x');
            for (int shift = 60; shift >= 0; shift -= 4) {
                unsigned d = (v >> shift) & 0xF;
                buf_putc(bufp, rem, hex[d]);
            }
            break;
        }
        case '%': buf_putc(bufp, rem, '%'); break;
        default: buf_putc(bufp, rem, '%'); buf_putc(bufp, rem, *p); break;
        }
    }
}

int ksnprintf(char *buf, size_t bufsz, const char *fmt, ...)
{
    if (bufsz == 0) return 0;
    char *b = buf;
    size_t rem = bufsz;
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(&b, &rem, fmt, ap);
    va_end(ap);
    /* ensure null termination */
    if (rem > 0) {
        *b = '\0';
    } else {
        buf[bufsz-1] = '\0';
    }
    return (int)(bufsz - rem);
}

void kclear(void)
{
    uint16_t fill = make_cell(' ');
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        vga[i] = fill;
    cx = cy = 0;
}

kputc_fn kprint_set_sink(kputc_fn fn)
{
    kputc_fn prev = sink;
    sink = fn;
    return prev;
}
kputc_fn kprint_get_sink(void) { return sink; }
