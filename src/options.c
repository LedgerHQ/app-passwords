#include <stdbool.h>
#include "os_nvm.h"

#include "options.h"
#include "globals.h"

static void set_charset_options(uint8_t value) {
    nvm_write((void *) &N_storage.charset_options, (void *) &value, sizeof(value));
}

uint8_t get_charset_options() {
    return N_storage.charset_options;
}

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
    nvm_write((void *) &N_storage.press_enter_after_typing, (void *) &new_value, sizeof(new_value));
}

/* `keyboard_layout` is a single byte followed by struct padding and then `metadata_count`.
 * Writing sizeof(hid_mapping_t) bytes there (an enum is 4 bytes on the ARM toolchain) used to
 * spill over the padding and clobber the first byte of the persistent password count, which a
 * plain layout change in the settings was enough to trigger. Keep the NVM write sized on the
 * destination field, and pin that assumption down. */
_Static_assert(sizeof(((internalStorage_t *) 0)->keyboard_layout) == 1,
               "keyboard_layout must stay a single byte, see set_keyboard_layout()");

bool set_keyboard_layout(hid_mapping_t mapping) {
    switch (mapping) {
        case HID_MAPPING_NONE:
        case HID_MAPPING_QWERTY:
        case HID_MAPPING_QWERTY_INTL:
        case HID_MAPPING_AZERTY:
            break;
        default:
            // Never persist a layout map_char() cannot resolve.
            return false;
    }
    const bool return_value = (N_storage.keyboard_layout == HID_MAPPING_NONE);
    const uint8_t stored_mapping = (uint8_t) mapping;
    nvm_write((void *) &N_storage.keyboard_layout,
              (void *) &stored_mapping,
              sizeof(N_storage.keyboard_layout));
    return return_value;
}
