#pragma once

#include <stdbool.h>

void password_list_reset();

size_t password_list_get_offset(const size_t index);

const char *password_list_get_password(const size_t index);

bool password_list_add_password(const size_t index,
                                const size_t offset,
                                const char *const password,
                                const size_t length);

void password_list_set_current(const size_t index);

size_t password_list_get_current_offset();

/* Length of the common prefix of two entries' nicknames, ignoring the ASCII case. */
size_t password_list_common_prefix_len(const size_t index1, const size_t index2);

/* Sort the first `count` entries by nickname, ignoring the ASCII case. */
void password_list_sort(const size_t count);

void password_list_reset_buffer();

const char *const *password_list_passwords();
