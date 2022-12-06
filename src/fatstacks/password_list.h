#pragma once

#define DISPLAYED_PASSWORD_PER_PAGE 5

#if defined(TARGET_FATSTACKS)

void password_list_reset();

void password_list_set_index(const size_t index);

size_t password_list_get_index();

void password_list_incr_index();

void password_list_set_offset(const size_t index, const size_t offset);

size_t password_list_get_offset(const size_t index);

size_t password_list_get_current_offset();

void password_list_set_password(const size_t index, const char *const password);

const char *password_list_get_password(const size_t index);

char *password_list_buffer_ptr(const size_t offset);

void password_list_reset_buffer();

char **password_list_passwords();

#endif
