#include "password_options.h"


static uint8_t charsetOptions;


void init_charset_options() {
    charsetOptions = 0x07;  // default: uppercase, lowercase, numbers only
}


uint8_t get_charset_options() {
    return charsetOptions;
}


void set_charset_option(const uint8_t bitflag) {
    charsetOptions ^= bitflag;
}
