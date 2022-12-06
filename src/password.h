#pragma once

#include <stdlib.h>
#include <string.h>

#define PASSWORD_MAX_SIZE 20

error_type_t create_new_password(const char* const pwd_name, const size_t pwd_size);
void type_password_at_offset(const size_t offset);
void show_password_at_offset(const size_t offset, uint8_t* dest_buffer);
error_type_t delete_password_at_offset(const size_t offset);
