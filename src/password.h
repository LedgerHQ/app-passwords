#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

#define PASSWORD_MAX_SIZE 20

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
