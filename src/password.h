#pragma once

#include <stdlib.h>
#include <string.h>
#include "error.h"

#define PASSWORD_MAX_SIZE 20

/*
 * Inserts a new password into the storage.
 * `pwd_size` should not include the string last null-byte.
 */
error_type_t create_new_password(const char* const pwd_name, const size_t pwd_size);
void type_password_at_offset(const size_t offset);
void show_password_at_offset(const size_t offset, uint8_t* dest_buffer);
error_type_t delete_password_at_offset(const size_t offset);
