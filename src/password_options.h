#pragma once

#include <unistd.h>

enum {
    UPPERCASE_BITFLAG   = 1,
    LOWERCASE_BITFLAG   = 2,
    NUMBERS_BITFLAG     = 4,
    BARS_BITFLAG        = 8 | 16 | 32,
    EXT_SYMBOLS_BITFLAG = 64 | 128,
};


void init_charset_options(void);
uint8_t get_charset_options(void);
void set_charset_option(const uint8_t bitflag);
