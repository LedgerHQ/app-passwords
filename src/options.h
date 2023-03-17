#pragma once

#include <unistd.h>

#include <hid_mapping.h>

enum {
    UPPERCASE_BITFLAG = 1,
    LOWERCASE_BITFLAG = 2,
    NUMBERS_BITFLAG = 4,
    BARS_BITFLAG = 8 | 16 | 32,
    EXT_SYMBOLS_BITFLAG = 64 | 128,
};

void init_charset_options(void);
bool has_charset_option(const uint8_t bitflag);
uint8_t get_charset_options(void);
void set_charset_option(const uint8_t bitflag);
void change_enter_options();

#if !defined(TARGET_STAX)
/*
 * Store the keyboard layout in NVM
 * Returns if it's the first time a layout is stored (true) or not (false)
 */
bool storage_keyboard_layout(hid_mapping_t mapping);
#endif
