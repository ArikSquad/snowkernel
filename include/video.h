#pragma once
#include <stdint.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp; /* bits per pixel (we'll aim for 32) */
    uint32_t pitch; /* bytes per line */
    volatile uint8_t *lfb; /* linear framebuffer base */
    int available; /* 1 if initialized */
} video_mode_t;

int video_init(uint32_t want_w, uint32_t want_h, uint32_t want_bpp);
int video_enable_mode(uint32_t want_w, uint32_t want_h, uint32_t want_bpp);
const video_mode_t *video_mode(void);
void video_clear(uint32_t rgb);
void video_putpixel(int x,int y,uint32_t rgb);
void video_fillrect(int x,int y,int w,int h,uint32_t rgb);
void video_draw_char(int x,int y,char c,uint32_t fg,uint32_t bg);
void video_draw_text(int x,int y,const char *s,uint32_t fg,uint32_t bg);
void video_test_pattern(void);
