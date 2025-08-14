#include <stdbool.h>
#include "os_nvm.h"

#include "options.h"
#include "globals.h"

#if !defined(TARGET_STAX)

static uint8_t charset_options;

static void set_charset_options(uint8_t value) {
    charset_options = value;
}

uint8_t get_charset_options() {
    return charset_options;
}

#else

static void set_charset_options(uint8_t value) {
    nvm_write((void*) &N_storage.charset_options, (void*) &value, sizeof(value));
}

uint8_t get_charset_options() {
    return N_storage.charset_options;
}

#endif  // !defined(TARGET_STAX)

void init_charset_options() {
    // default: uppercase (1) + lowercase (2) + numbers (4) = 7
    set_charset_options(0x07);
}

bool has_charset_option(const uint8_t bitflag) {
    return (get_charset_options() & bitflag) != 0;
}

void set_charset_option(const uint8_t bitflag) {
    set_charset_options(get_charset_options() ^ bitflag);
}

void change_enter_options() {
    bool new_value = !N_storage.press_enter_after_typing;
    nvm_write((void*) &N_storage.press_enter_after_typing, (void*) &new_value, sizeof(new_value));
}

bool set_keyboard_layout(hid_mapping_t mapping) {
    const bool return_value = (N_storage.keyboard_layout == 0);
    nvm_write((void*) &N_storage.keyboard_layout, (void*) &mapping, sizeof(hid_mapping_t));
    return return_value;
}
