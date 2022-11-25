#include <os_nvm.h>

#include "password_options.h"
#include "globals.h"

#if !defined(TARGET_FATSTACKS)

static uint8_t charset_options;

void init_charset_options() {
    charset_options = 0x07;  // default: uppercase, lowercase, numbers only
}

uint8_t get_charset_options() {
    return charset_options;
}

bool has_charset_option(const uint8_t bitflag) {
    return (charset_options & bitflag) != 0;
}

void set_charset_option(const uint8_t bitflag) {
    charset_options ^= bitflag;
}

#else

void init_charset_options() {
    uint8_t value = 0x07;
    nvm_write(&N_storage.charset_options, (void*) &value, sizeof(value));  // default: uppercase, lowercase, numbers only
}

uint8_t get_charset_options() {
    return N_storage.charset_options;
}

bool has_charset_option(const uint8_t bitflag) {
    PRINTF("All: '%d' check with '%d', result '%d'\n", N_storage.charset_options, bitflag, N_storage.charset_options & bitflag);
    return (N_storage.charset_options & bitflag) != 0;
}

void set_charset_option(const uint8_t bitflag) {
    uint8_t value = N_storage.charset_options ^ bitflag;
    nvm_write(&N_storage.charset_options, (void*) &value, sizeof(value));
}

#endif
