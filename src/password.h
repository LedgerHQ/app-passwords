#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

// Length of version 0 passwords, and default length of new ones.
#define PASSWORD_DEFAULT_SIZE 20
#define PASSWORD_MAX_SIZE     64

/* Lengths offered for new passwords. */
static inline bool password_length_is_supported(uint8_t length) {
    switch (length) {
        case 8:
        case 12:
        case 16:
        case 20:
        case 24:
        case 32:
        case 48:
        case 64:
            return true;
        default:
            return false;
    }
}

/*
 * Inserts a new password into the storage.
 * `pwd_size` should not include the string last null-byte.
 */
error_type_t create_new_password(const char *const pwd_name, const size_t pwd_size);
/*
 * Generate the password for the entry at `offset`. `type_password_at_offset` types it on the
 * host keyboard; `show_password_at_offset` writes it into `dest_buffer`, which must hold
 * PASSWORD_MAX_SIZE + 1 bytes and is zeroed on failure. Both answer false when derivation or
 * generation failed, in which case nothing was typed and no password is available: the caller
 * must not report success.
 */
bool type_password_at_offset(const size_t offset);
bool show_password_at_offset(const size_t offset, uint8_t *dest_buffer);
error_type_t delete_password_at_offset(const size_t offset);
bool nickname_exists(const char *const pwd_name, const size_t pwd_size);
