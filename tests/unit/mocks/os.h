#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PRINTF printf

// Declared in os_nvm.h by the SDK. metadata.c calls it; tests provide the
// implementation as a stub. Keep the signature in sync with the SDK
// (`unsigned int src_len`).
void nvm_write(void *dst_adr, void *src_adr, unsigned int src_len);
#ifndef THROW
#define THROW(X) return
#endif
#ifndef EXCEPTION
#define EXCEPTION 1
#endif
// Same as the SDK's is_printable_string() (os_utils.h): ASCII 0x20 to 0x7E, an empty string is
// printable.
static inline bool is_printable_string(const char *str, size_t len) {
    if (len == 0) {
        return true;
    }
    if (str == NULL) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (((unsigned char) str[i] < 0x20) || ((unsigned char) str[i] > 0x7E)) {
            return false;
        }
    }
    return true;
}

// SDK macro that turns a relative pointer into an absolute one on device.
// In host-side unit tests pointers are already absolute, so it's a no-op.
#define PIC(x) (x)

// Declared `static inline` so each translation unit that pulls in this mock
// header gets its own copy without producing duplicate-symbol link errors.
static inline bool bolos_ux_mnemonic_check(const unsigned char *buffer, unsigned int length) {
    const char *expected_mnemonic = "list of random words which actually are the mnemonic";
    printf("Comparing strings under size '%d'\n", length);
    printf(" - expected: '%s'\n", expected_mnemonic);
    printf(" - given:    '%s'\n", buffer);
    return (strncmp(expected_mnemonic, (const char *) buffer, length) == 0);
}
