#pragma once

#include <stdbool.h>
#include <unistd.h>

#include "hid_mapping.h"

enum {
    UPPERCASE_BITFLAG = 1,
    LOWERCASE_BITFLAG = 2,
    NUMBERS_BITFLAG = 4,
    BARS_BITFLAG = 8 | 16 | 32,
    EXT_SYMBOLS_BITFLAG = 64 | 128,
};

void init_charset_options(void);

/* Length of new passwords: the stored value if it is a supported length, the default otherwise. */
uint8_t get_password_length(void);
/* Stores `length` if it is a supported length. Returns whether it was stored. */
bool set_password_length(uint8_t length);
bool has_charset_option(const uint8_t bitflag);
uint8_t get_charset_options(void);
void set_charset_option(const uint8_t bitflag);
void change_enter_options();

/* Order of the passwords list, stored on one byte. Any unknown value reads as creation order. */
typedef enum {
    PASSWORDS_ORDER_CREATION = 0,
    PASSWORDS_ORDER_ALPHABETICAL = 1,
} passwords_order_e;
bool passwords_sorted_alphabetically(void);
void change_passwords_order(void);

/*
 * Store the keyboard layout in NVM
 * Returns if it's the first time a layout is stored (true) or not (false)
 */
bool set_keyboard_layout(hid_mapping_t mapping);
