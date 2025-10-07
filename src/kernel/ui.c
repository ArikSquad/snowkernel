#include "ui.h"
#include "kprint.h"
#include "../drivers/keyboard.h"
#include "video.h"
#include "mouse.h"
#include <stdint.h>

extern volatile uint64_t ticks; /* from irq_timer.c */

static void draw_frame(void)
{
    kclear();
}

static void update_clock(void)
{
    uint64_t t = ticks;
    uint64_t seconds = t / 100; // assume 100Hz PIT; adjust later
    uint64_t mins = seconds / 60;
    uint64_t hrs = mins / 60;
    seconds %= 60;
    mins %= 60;

    kprintf("\r[ Snow UI ]  Time: %02u:%02u:%02u  Apps: [Shell] [Info]  Keys: q=exit", (unsigned)hrs, (unsigned)mins, (unsigned)seconds);
}

typedef struct window
{
    int x, y, w, h;
    const char *title;
    int needs_redraw;
} window_t;

static void draw_window_frame(window_t *win, uint32_t frame, uint32_t title_bar, uint32_t title_fg, uint32_t bg)
{
    for (int dx = 0; dx < win->w; dx++)
    {
        video_putpixel(win->x + dx, win->y, frame);
        video_putpixel(win->x + dx, win->y + win->h - 1, frame);
    }
    for (int dy = 0; dy < win->h; dy++)
    {
        video_putpixel(win->x, win->y + dy, frame);
        video_putpixel(win->x + win->w - 1, win->y + dy, frame);
    }

    video_fillrect(win->x + 1, win->y + 16, win->w - 2, win->h - 17, bg);
    video_fillrect(win->x + 1, win->y + 1, win->w - 2, 14, title_bar);
    video_draw_text(win->x + 6, win->y + 3, win->title, title_fg, 0xFFFFFFFF);
}

static void draw_menu_dropdown(int open)
{
    if (!open)
        return;

    video_fillrect(0, 24, 140, 90, 0x2A2A40);
    video_draw_text(8, 30, "Menu", 0xFFFFFF, 0x2A2A40);
    video_draw_text(8, 46, "1) Shutdown", 0xCCCCCC, 0x2A2A40);
    video_draw_text(8, 62, "2) Reboot", 0xCCCCCC, 0x2A2A40);
    video_draw_text(8, 78, "q) Exit UI", 0xCCCCCC, 0x2A2A40);
}

static window_t term_win = {.x = 80, .y = 120, .w = 640, .h = 400, .title = "Terminal", .needs_redraw = 1};

#define TERM_COLS ((640 - 2 - 8) / 8)
#define TERM_ROWS ((400 - 16 - 4 - 8) / 16)
static char term_buf[TERM_ROWS][TERM_COLS];
static int term_cx = 0, term_cy = 0;

static void term_clear(void)
{
    for (int r = 0; r < TERM_ROWS; r++)
        for (int c = 0; c < TERM_COLS; c++)
            term_buf[r][c] = '\0';
    term_cx = term_cy = 0;
}

static void term_draw(void)
{
    const int ox = term_win.x + 4;
    const int oy = term_win.y + 16 + 4;
    for (int r = 0; r < TERM_ROWS; r++)
    {
        for (int c = 0; c < TERM_COLS; c++)
        {
            char ch = term_buf[r][c];
            if (!ch)
                ch = ' ';
            video_draw_char(ox + c * 8, oy + r * 16, ch, 0xE0E0E0, 0x000000);
        }
    }
}

static void term_putc(char c)
{
    if (c == '\n')
    {
        term_cx = 0;
        if (++term_cy >= TERM_ROWS)
        { /* scroll */
            for (int r = 1; r < TERM_ROWS; r++)
                for (int c2 = 0; c2 < TERM_COLS; c2++)
                    term_buf[r - 1][c2] = term_buf[r][c2];
            for (int c2 = 0; c2 < TERM_COLS; c2++)
                term_buf[TERM_ROWS - 1][c2] = '\0';
            term_cy = TERM_ROWS - 1;
        }
        return;
    }
    if (c == '\r')
    {
        term_cx = 0;
        return;
    }
    if (c == '\t')
    {
        int spaces = 4 - (term_cx & 3);
        while (spaces--)
            term_putc(' ');
        return;
    }
    if (term_cx >= TERM_COLS)
    {
        term_putc('\n');
    }
    term_buf[term_cy][term_cx++] = c;
}

void ui_run(void)
{
    extern int video_enable_mode(uint32_t, uint32_t, uint32_t);
    video_enable_mode(1024, 768, 32);
    const video_mode_t *m = video_mode();
    if (m && m->available)
        mouse_init();
    int menu_open = 0;
    if (m && m->available)
    {
        video_clear(0x202030);                        /* dark background */
        video_fillrect(0, 0, m->width, 24, 0x303860); /* app bar */
        video_fillrect(0, 0, 60, 24, 0x404880);       /* menu button region */
        video_draw_text(8, 4, "Menu", 0xFFFFFF, 0x404880);
        video_draw_text(80, 4, "Snow UI - press q to exit", 0xFFFFFF, 0x303860);
        term_win.needs_redraw = 1;
        draw_window_frame(&term_win, 0x101018, 0x404860, 0xFFFFFF, 0x000000);
    }
    else
    {
        draw_frame();
    }
    uint64_t last_tick = 0;

    int sel_x = 0, sel_y = 0;
    const int icon_w = 64, icon_h = 48;
    const int icons_cols = 4;
    const char *icons[] = {"Terminal", "Files", "Settings", "Info"};
    const int icons_n = 4;

    if (m && m->available)
    {
        for (int i = 0; i < icons_n; i++)
        {
            int col = i % icons_cols;
            int row = i / icons_cols;
            int x = 16 + col * (icon_w + 16);
            int y = 40 + row * (icon_h + 16);
            video_fillrect(x, y, icon_w, icon_h, 0x383838);
            video_draw_text(x + 8, y + 16, icons[i], 0xE0E0E0, 0x383838);
        }
        draw_menu_dropdown(menu_open);
    }
    for (;;)
    {
        if (m && m->available)
        {
            int cx, cy;
            mouse_get_position(&cx, &cy);
            /* simple arrow */
            static const uint16_t pattern[12] = {0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xFF00, 0xFF80, 0xF800, 0xD800, 0x8800};
            for (int r = 0; r < 12; r++)
            {
                uint16_t bits = pattern[r];
                for (int b = 0; b < 16; b++)
                    if (bits & (0x8000 >> b))
                        video_putpixel(cx + b, cy + r, 0xFFFFFF);
            }
        }
        if (ticks - last_tick >= 100)
        { // update once per second
            if (m && m->available)
            {
                uint64_t t = ticks / 100;
                uint64_t s = t % 60;
                uint64_t mnt = (t / 60) % 60;
                uint64_t h = (t / 3600);
                char buf[64];
                ksnprintf(buf, sizeof(buf), "%02u:%02u:%02u", (unsigned)h, (unsigned)mnt, (unsigned)s);
                // Redraw time area
                video_fillrect(300, 0, 120, 24, 0x303860);
                video_draw_text(300, 4, buf, 0xFFFFFF, 0x303860);
            }
            else
            {
                update_clock();
            }
            last_tick = ticks - ((ticks - last_tick) % 100);
        }
        int v = kbd_getc_nonblock();
        if (v >= 0)
        {
            if (v == 'q')
            {
                if (m && m->available)
                    kprintf("\n[ui] exit (fb)\n");
                else
                    kprintf("\n[ui] exit\n");
                return;
            }
            if (v == 'm')
            { /* toggle menu */
                if (m && m->available)
                {
                    menu_open = !menu_open;
                    /* redraw bar */
                    video_fillrect(0, 0, m->width, 24, 0x303860);
                    video_fillrect(0, 0, 60, 24, 0x404880);
                    video_draw_text(8, 4, "Menu", 0xFFFFFF, 0x404880);
                    video_draw_text(80, 4, "Snow UI - press q to exit", 0xFFFFFF, 0x303860);
                    draw_window_frame(&term_win, 0x101018, 0x404860, 0xFFFFFF, 0x000000);
                    draw_menu_dropdown(menu_open);
                }
            }
            if (menu_open && (v == '1' || v == '2'))
            {
                if (v == '1')
                {
                    extern void power_shutdown(void);
                    power_shutdown();
                }
                if (v == '2')
                {
                    extern void power_reboot(void);
                    power_reboot();
                }
            }

            if (v == 't')
            {
                kprintf("\n[ui] launching Terminal\n");
                extern void shell_run(void);
                shell_run();
                if (m && m->available)
                {
                    video_clear(0x202030);
                    video_fillrect(0, 0, m->width, 24, 0x303860);
                    video_fillrect(0, 0, 60, 24, 0x404880);
                    video_draw_text(8, 4, "Menu", 0xFFFFFF, 0x404880);
                    video_draw_text(80, 4, "Snow UI - press q to exit", 0xFFFFFF, 0x303860);
                    for (int i = 0; i < icons_n; i++)
                    {
                        int col = i % icons_cols;
                        int row = i / icons_cols;
                        int x = 16 + col * (icon_w + 16);
                        int y = 40 + row * (icon_h + 16);
                        video_fillrect(x, y, icon_w, icon_h, 0x383838);
                        video_draw_text(x + 8, y + 16, icons[i], 0xE0E0E0, 0x383838);
                    }
                    draw_window_frame(&term_win, 0x101018, 0x404860, 0xFFFFFF, 0x000000);
                    draw_menu_dropdown(menu_open);
                }
                else
                {
                    draw_frame();
                }
            }
        }
        /* Mouse click handling for menu button */
        if (m && m->available)
        {
            int cx, cy;
            mouse_get_position(&cx, &cy);
            int buttons = mouse_get_buttons();
            if (buttons & 1)
            {
                if (cy < 24 && cx < 60)
                { /* menu region */
                    /* simulate 'm' key toggle */
                    menu_open = !menu_open;
                    video_fillrect(0, 0, m->width, 24, 0x303860);
                    video_fillrect(0, 0, 60, 24, 0x404880);
                    video_draw_text(8, 4, "Menu", 0xFFFFFF, 0x404880);
                    video_draw_text(80, 4, "Snow UI - press q to exit", 0xFFFFFF, 0x303860);
                    draw_window_frame(&term_win, 0x101018, 0x404860, 0xFFFFFF, 0x000000);
                    draw_menu_dropdown(menu_open);
                }
            }
        }
    }
}
