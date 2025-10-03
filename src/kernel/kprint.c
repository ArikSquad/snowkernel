#include "kprint.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)

static uint16_t* const vga = VGA_MEMORY;
static size_t cx = 0, cy = 0;
static uint8_t color = 0x07;
static kputc_fn sink = 0;

static void scroll() {
    if (cy < VGA_HEIGHT) return;
    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            vga[(y-1)*VGA_WIDTH + x] = vga[y*VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; ++x) vga[(VGA_HEIGHT-1)*VGA_WIDTH + x] = ((uint16_t)color << 8) | ' ';
    cy = VGA_HEIGHT - 1;
}

static void newline() {
    cx = 0;
    cy++;
    scroll();
}

void kset_color(uint8_t fg, uint8_t bg) { color = (bg << 4) | (fg & 0x0F); }

static void vga_putc(char c){
    if (c == '\n') { newline(); return; }
    if (c == '\r') { cx = 0; return; }
    if (c == '\b') { if (cx>0) { cx--; vga[cy*VGA_WIDTH + cx] = ((uint16_t)color << 8) | ' '; } return; }
    if (c == '\t') { size_t n = 4 - (cx & 3); while (n--) vga_putc(' '); return; }
    vga[cy*VGA_WIDTH + cx] = ((uint16_t)color << 8) | (uint8_t)c;
    if (++cx >= VGA_WIDTH) { newline(); }
}

void kputc(char c){ if(sink) sink(c); else vga_putc(c); }

void kputs(const char* s) { while (*s) kputc(*s++); }

static void kvprintf(const char* fmt, va_list ap) {
    for (const char* p = fmt; *p; ++p) {
        if (*p != '%') { kputc(*p); continue; }
        ++p;
        switch (*p) {
            case 's': { const char* s = va_arg(ap, const char*); if (!s) s = "(null)"; kputs(s); break; }
            case 'c': { char c = (char)va_arg(ap, int); kputc(c); break; }
            case 'd': {
                int v = va_arg(ap, int);
                if (v < 0) { kputc('-'); v = -v; }
                char buf[16]; int i = 0; do { buf[i++] = '0' + (v % 10); v /= 10; } while (v && i < 15);
                while (i--) kputc(buf[i]);
                break; }
            case 'u': {
                unsigned v = va_arg(ap, unsigned);
                char buf[16]; int i = 0; do { buf[i++] = '0' + (v % 10); v /= 10; } while (v && i < 15);
                while (i--) kputc(buf[i]);
                break; }
            case 'x': {
                unsigned v = va_arg(ap, unsigned);
                char* hex = "0123456789abcdef";
                char buf[16]; int i = 0; do { buf[i++] = hex[v & 15]; v >>= 4; } while (v && i < 15);
                if (i==0) kputc('0'); else while (i--) kputc(buf[i]);
                break; }
            case '%': kputc('%'); break;
            default: kputc('%'); kputc(*p); break;
        }
    }
}

void kprintf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); kvprintf(fmt, ap); va_end(ap);
}

void kclear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y)
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            vga[y*VGA_WIDTH + x] = ((uint16_t)color << 8) | ' ';
    cx = cy = 0;
}

kputc_fn kprint_set_sink(kputc_fn fn){ kputc_fn prev=sink; sink=fn; return prev; }
kputc_fn kprint_get_sink(void){ return sink; }
