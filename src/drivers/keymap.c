#include "keymap.h"
#include <stdint.h>

#define KBD_CHAR_BUF 128
static char char_buf[KBD_CHAR_BUF];
static uint8_t char_head = 0; // next write
static uint8_t char_tail = 0; // next read

static inline int buf_is_full(void) { return (uint8_t)((char_head + 1) % KBD_CHAR_BUF) == char_tail; }
static inline int buf_is_empty(void) { return char_head == char_tail; }
static inline void buf_push(char c)
{
    uint8_t next = (uint8_t)((char_head + 1) % KBD_CHAR_BUF);
    if (next == char_tail)
    { // overwrite oldest
        char_tail = (uint8_t)((char_tail + 1) % KBD_CHAR_BUF);
    }
    char_buf[char_head] = c;
    char_head = next;
}
static inline int buf_pop(void)
{
    if (buf_is_empty())
        return -1;
    char c = char_buf[char_tail];
    char_tail = (uint8_t)((char_tail + 1) % KBD_CHAR_BUF);
    return (unsigned char)c;
}

static uint8_t modifiers = 0; // bitset of KMOD_*
static uint8_t extended_prefix = 0;
static keymap_id_t active_layout = KEYMAP_US;

static int initialized = 0;
typedef struct
{
    const char normal[128];
    const char shift[128];
} layout_t;

#define _N 0

static const layout_t layout_us = {
    .normal = {
        [1] = 27,
        [2] = '1',
        [3] = '2',
        [4] = '3',
        [5] = '4',
        [6] = '5',
        [7] = '6',
        [8] = '7',
        [9] = '8',
        [10] = '9',
        [11] = '0',
        [12] = '-',
        [13] = '=',
        [14] = 8,
        [15] = 9,
        [16] = 'q',
        [17] = 'w',
        [18] = 'e',
        [19] = 'r',
        [20] = 't',
        [21] = 'y',
        [22] = 'u',
        [23] = 'i',
        [24] = 'o',
        [25] = 'p',
        [26] = '[',
        [27] = ']',
        [28] = 0,
        [29] = 0,
        [30] = 'a',
        [31] = 's',
        [32] = 'd',
        [33] = 'f',
        [34] = 'g',
        [35] = 'h',
        [36] = 'j',
        [37] = 'k',
        [38] = 'l',
        [39] = ';',
        [40] = '\'',
        [41] = '`',
        [42] = 0,
        [43] = '\\',
        [44] = 'z',
        [45] = 'x',
        [46] = 'c',
        [47] = 'v',
        [48] = 'b',
        [49] = 'n',
        [50] = 'm',
        [51] = ',',
        [52] = '.',
        [53] = '/',
        [54] = 0,
        [55] = '*',
        [56] = 0,
        [57] = ' ',
    },
    .shift = {
        [1] = 27,
        [2] = '!',
        [3] = '@',
        [4] = '#',
        [5] = '$',
        [6] = '%',
        [7] = '^',
        [8] = '&',
        [9] = '*',
        [10] = '(',
        [11] = ')',
        [12] = '_',
        [13] = '+',
        [14] = 8,
        [15] = 9,
        [16] = 'Q',
        [17] = 'W',
        [18] = 'E',
        [19] = 'R',
        [20] = 'T',
        [21] = 'Y',
        [22] = 'U',
        [23] = 'I',
        [24] = 'O',
        [25] = 'P',
        [26] = '{',
        [27] = '}',
        [28] = 0,
        [29] = 0,
        [30] = 'A',
        [31] = 'S',
        [32] = 'D',
        [33] = 'F',
        [34] = 'G',
        [35] = 'H',
        [36] = 'J',
        [37] = 'K',
        [38] = 'L',
        [39] = ':',
        [40] = '"',
        [41] = '~',
        [42] = 0,
        [43] = '|',
        [44] = 'Z',
        [45] = 'X',
        [46] = 'C',
        [47] = 'V',
        [48] = 'B',
        [49] = 'N',
        [50] = 'M',
        [51] = '<',
        [52] = '>',
        [53] = '?',
        [54] = 0,
        [55] = '*',
        [56] = 0,
        [57] = ' ',
    }};

static const layout_t layout_dvorak = {
    .normal = {
        [1] = 27,
        [2] = '1',
        [3] = '2',
        [4] = '3',
        [5] = '4',
        [6] = '5',
        [7] = '6',
        [8] = '7',
        [9] = '8',
        [10] = '9',
        [11] = '0',
        [12] = '[',
        [13] = ']',
        [14] = 8,
        [15] = 9,
        [16] = '\'',
        [17] = ',',
        [18] = '.',
        [19] = 'p',
        [20] = 'y',
        [21] = 'f',
        [22] = 'g',
        [23] = 'c',
        [24] = 'r',
        [25] = 'l',
        [26] = '/',
        [27] = '=',
        [28] = 0,
        [29] = 0,
        [30] = 'a',
        [31] = 'o',
        [32] = 'e',
        [33] = 'u',
        [34] = 'i',
        [35] = 'd',
        [36] = 'h',
        [37] = 't',
        [38] = 'n',
        [39] = 's',
        [40] = '-',
        [41] = '`',
        [42] = 0,
        [43] = '\\',
        [44] = ';',
        [45] = 'q',
        [46] = 'j',
        [47] = 'k',
        [48] = 'x',
        [49] = 'b',
        [50] = 'm',
        [51] = 'w',
        [52] = 'v',
        [53] = 'z',
        [54] = 0,
        [55] = '*',
        [56] = 0,
        [57] = ' ',
    },
    .shift = {
        [1] = 27,
        [2] = '!',
        [3] = '@',
        [4] = '#',
        [5] = '$',
        [6] = '%',
        [7] = '^',
        [8] = '&',
        [9] = '*',
        [10] = '(',
        [11] = ')',
        [12] = '{',
        [13] = '}',
        [14] = 8,
        [15] = 9,
        [16] = '"',
        [17] = '<',
        [18] = '>',
        [19] = 'P',
        [20] = 'Y',
        [21] = 'F',
        [22] = 'G',
        [23] = 'C',
        [24] = 'R',
        [25] = 'L',
        [26] = '?',
        [27] = '+',
        [28] = 0,
        [29] = 0,
        [30] = 'A',
        [31] = 'O',
        [32] = 'E',
        [33] = 'U',
        [34] = 'I',
        [35] = 'D',
        [36] = 'H',
        [37] = 'T',
        [38] = 'N',
        [39] = 'S',
        [40] = '_',
        [41] = '~',
        [42] = 0,
        [43] = '|',
        [44] = ':',
        [45] = 'Q',
        [46] = 'J',
        [47] = 'K',
        [48] = 'X',
        [49] = 'B',
        [50] = 'M',
        [51] = 'W',
        [52] = 'V',
        [53] = 'Z',
        [54] = 0,
        [55] = '*',
        [56] = 0,
        [57] = ' ',
    }};

// Finnish layout (CP437 codes for umlauts): ä=0x84, ö=0x94, å=0x86, Ä=0x8E, Ö=0x99, Å=0x8F
static const layout_t layout_fi = {
    .normal = {
        [1] = 27,
        [2] = '1',
        [3] = '2',
        [4] = '3',
        [5] = '4',
        [6] = '5',
        [7] = '6',
        [8] = '7',
        [9] = '8',
        [10] = '9',
        [11] = '0',
        [12] = '-',
        [13] = '=',
        [14] = 8,
        [15] = 9,
        [16] = 'q',
        [17] = 'w',
        [18] = 'e',
        [19] = 'r',
        [20] = 't',
        [21] = 'y',
        [22] = 'u',
        [23] = 'i',
        [24] = 'o',
        [25] = 'p',
        [26] = '[',
        [27] = ']',
        [28] = 0,
        [29] = 0,
        [30] = 'a',
        [31] = 's',
        [32] = 'd',
        [33] = 'f',
        [34] = 'g',
        [35] = 'h',
        [36] = 'j',
        [37] = 'k',
        [38] = 'l',
        [39] = '\x84', // ä
        [40] = '\x94', // ö
        [41] = '\x86', // å
        [42] = 0,
        [43] = '\\',
        [44] = 'z',
        [45] = 'x',
        [46] = 'c',
        [47] = 'v',
        [48] = 'b',
        [49] = 'n',
        [50] = 'm',
        [51] = ',',
        [52] = '.',
        [53] = '/',
        [54] = 0,
        [55] = '*',
        [56] = 0,
        [57] = ' ',
    },
    .shift = {
        [1] = 27,
        [2] = '!',
        [3] = '"',
        [4] = '#',
        [5] = '$',
        [6] = '%',
        [7] = '&',
        [8] = '/',
        [9] = '(',
        [10] = ')',
        [11] = '=',
        [12] = '?',
        [13] = '`',
        [14] = 8,
        [15] = 9,
        [16] = 'Q',
        [17] = 'W',
        [18] = 'E',
        [19] = 'R',
        [20] = 'T',
        [21] = 'Y',
        [22] = 'U',
        [23] = 'I',
        [24] = 'O',
        [25] = 'P',
        [26] = '{',
        [27] = '}',
        [28] = 0,
        [29] = 0,
        [30] = 'A',
        [31] = 'S',
        [32] = 'D',
        [33] = 'F',
        [34] = 'G',
        [35] = 'H',
        [36] = 'J',
        [37] = 'K',
        [38] = 'L',
        [39] = '\x8E', // Ä
        [40] = '\x99', // Ö
        [41] = '\x8F', // Å
        [42] = 0,
        [43] = '|',
        [44] = 'Z',
        [45] = 'X',
        [46] = 'C',
        [47] = 'V',
        [48] = 'B',
        [49] = 'N',
        [50] = 'M',
        [51] = ';',
        [52] = ';',
        [53] = ':',
        [54] = 0,
        [55] = '*',
        [56] = 0,
        [57] = ' ',
    }};

static const layout_t *layouts[KEYMAP_MAX] = {
    &layout_us,
    &layout_dvorak,
    &layout_fi};

void keymap_init(void)
{
    if (initialized)
        return;
    initialized = 1;
    active_layout = KEYMAP_US;
    modifiers = 0;
    extended_prefix = 0;
    char_head = char_tail = 0;
}

void keymap_set_layout(keymap_id_t id)
{
    if (id >= KEYMAP_MAX)
        id = KEYMAP_US;
    active_layout = id;
}

keymap_id_t keymap_get_layout(void)
{
    return active_layout;
}

uint8_t keymap_get_modifiers(void)
{
    return modifiers;
}

static void update_modifier(uint8_t sc, int released)
{
    switch (sc)
    {
    case 0x2A: // LShift
    case 0x36: // RShift
        if (released)
            modifiers &= ~KMOD_SHIFT;
        else
            modifiers |= KMOD_SHIFT;
        break;
    case 0x1D: // Ctrl (left)
        if (released)
            modifiers &= ~KMOD_CTRL;
        else
            modifiers |= KMOD_CTRL;
        break;
    case 0x38: // Alt (left)
        if (released)
            modifiers &= ~KMOD_ALT;
        else
            modifiers |= KMOD_ALT;
        break;
    case 0x3A: // CapsLock (toggle on press only)
        if (!released)
            modifiers ^= KMOD_CAPS;
        break;
    default:
        break;
    }
}

static void produce_ascii(uint8_t sc)
{
    const layout_t *lay = layouts[active_layout];
    char base = lay->normal[sc];
    if (!base)
        return;
    char out = 0;
    // Detect CP437 Finnish extended letters: ä(0x84), ö(0x94), å(0x86), Ä(0x8E), Ö(0x99), Å(0x8F)
    int is_cp437_umlaut = (unsigned char)base == 0x84 || (unsigned char)base == 0x94 || (unsigned char)base == 0x86 ||
                          (unsigned char)base == 0x8E || (unsigned char)base == 0x99 || (unsigned char)base == 0x8F;
    int is_alpha = (base >= 'a' && base <= 'z') || (base >= 'A' && base <= 'Z') || is_cp437_umlaut;
    if (is_alpha)
    {
        if (base >= 'A' && base <= 'Z')
            base = (char)(base - 'A' + 'a');
        int upper = ((modifiers & KMOD_SHIFT) ? 1 : 0) ^ ((modifiers & KMOD_CAPS) ? 1 : 0);
        if (upper)
            out = (char)(base - 'a' + 'A');
        else
            out = base;
    }
    else
    {
        if (modifiers & KMOD_SHIFT)
        {
            char shifted = lay->shift[sc];
            if (shifted)
                out = shifted;
            else
                out = base;
        }
        else
        {
            out = base;
        }
    }

    if (out)
    {
        unsigned char u = (unsigned char)out;
        if ((u >= 32 && u <= 126))
            buf_push(out);
        // Accept limited CP437 letters (Finnish) 0x84,0x94,0x86,0x8E,0x99,0x8F
        else if (u == 0x84 || u == 0x94 || u == 0x86 || u == 0x8E || u == 0x99 || u == 0x8F)
            buf_push(out);
    }
}

void keymap_process_scancode(uint8_t sc)
{
    keymap_init();

    if (sc == 0xE0)
    {
        extended_prefix = 1;
        return;
    }
    int released = (sc & 0x80) != 0;
    sc &= 0x7F; // base make code
    if (extended_prefix)
    {
        extended_prefix = 0;
        if (!released)
        {
            // kinda could have those escape sequences heree
        }
        return;
    }
    update_modifier(sc, released);
    if (released)
        return; // Only produce ASCII on make

    switch (sc)
    {
    case 0x0E: // Backspace
        buf_push(8);
        return;
    case 0x0F: // Tab
        buf_push('\t');
        return;
    case 0x1C: // Enter
        buf_push('\n');
        return;
    case 0x39: // Space
        buf_push(' ');
        return;
    default:
        break;
    }
    if (sc < 128)
        produce_ascii(sc);
}

int keymap_get_char(void)
{
    return buf_pop();
}
