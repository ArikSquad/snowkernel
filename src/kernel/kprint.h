#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*kputc_fn)(char c);
void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);
void kclear(void);
void kset_color(uint8_t fg, uint8_t bg);
kputc_fn kprint_set_sink(kputc_fn fn);
kputc_fn kprint_get_sink(void);
void kprint_enable_serial(void);
int ksnprintf(char *buf, size_t bufsz, const char *fmt, ...);
