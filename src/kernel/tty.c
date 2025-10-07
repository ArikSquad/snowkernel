#include "tty.h"
#include "kprint.h"
#include "../fs/fs.h"

static tty_t ttys[TTY_MAX];
static int active_tty = 0;

static inline uint16_t ring_next(uint16_t v) { return (uint16_t)((v + 1) % TTY_BUF); }

void tty_init(void) {
    for (int i=0;i<TTY_MAX;i++) {
        ttys[i].index = i;
        ttys[i].in_head = ttys[i].in_tail = 0;
        ttys[i].foreground = (i==0);
    }
}

tty_t *tty_get(int idx) { if (idx<0 || idx>=TTY_MAX) return 0; return &ttys[idx]; }

tty_t *tty_active(void) { return &ttys[active_tty]; }

void tty_set_active(int idx) { if (idx>=0 && idx<TTY_MAX) { active_tty = idx; for (int i=0;i<TTY_MAX;i++) ttys[i].foreground = (i==idx); } }

void tty_kbd_putc(char c) {
    tty_t *t = tty_active();
    uint16_t nh = ring_next(t->in_head);
    if (nh == t->in_tail) {
        // buffer full, drop char
        return;
    }
    t->inbuf[t->in_head] = c;
    t->in_head = nh;
}

int tty_read(tty_t *t, char *buf, size_t count) {
    if (!t) return -1;
    size_t r = 0;
    while (r < count && t->in_tail != t->in_head) {
        buf[r++] = t->inbuf[t->in_tail];
        t->in_tail = ring_next(t->in_tail);
    }
    return (int)r;
}

int tty_write(tty_t *t, const char *buf, size_t count) {
    (void)t; // currently single display; just print to kprint
    for (size_t i=0;i<count;i++) kputc(buf[i]);
    return (int)count;
}

void tty_register_fs(void) {
    node_t *dev = fs_find_child(fs_root(), "dev");
    if (!dev) dev = fs_mkdir(fs_root(), "dev");
    // create /dev/tty0 mapping to ttys[0]
    fs_create_chardev(dev, "tty0", &ttys[0]);
}
