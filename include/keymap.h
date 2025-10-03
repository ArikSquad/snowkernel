#pragma once
#include <stdint.h>

typedef enum
{
    KEYMAP_US = 0,
    KEYMAP_DVORAK = 1,
    KEYMAP_FI = 2,
    KEYMAP_MAX
} keymap_id_t;

#define KMOD_SHIFT (1u << 0)
#define KMOD_CTRL (1u << 1)
#define KMOD_ALT (1u << 2)
#define KMOD_CAPS (1u << 3)

typedef struct
{
    uint8_t scancode;  // raw scancode (set 1)
    uint8_t released;  // 0=press,1=release
    uint8_t modifiers; // current modifier state after this event
    uint16_t keycode;  // future use (logical key id)
    char ascii;
} key_event_t;

void keymap_init(void);
void keymap_set_layout(keymap_id_t id);
keymap_id_t keymap_get_layout(void);
void keymap_process_scancode(uint8_t sc);
int keymap_get_char(void);
uint8_t keymap_get_modifiers(void);
