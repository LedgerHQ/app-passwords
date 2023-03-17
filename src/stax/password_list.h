#pragma once

#include <stdbool.h>

#define DISPLAYED_PASSWORD_PER_PAGE 5

#if defined(TARGET_STAX)

void password_list_reset();

size_t password_list_get_offset(const size_t index);

const char *password_list_get_password(const size_t index);

bool password_list_add_password(const size_t index,
                                const size_t offset,
                                const char *const password,
                                const size_t length);

void password_list_set_current(const size_t index);

size_t password_list_get_current_offset();

void password_list_reset_buffer();

char **password_list_passwords();

#endif
