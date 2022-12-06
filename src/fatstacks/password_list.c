#include <strings.h>

#include "password_list.h"

typedef struct passwordList_s {
    /* Buffer where password name strings are stored */
    char buffer[DISPLAYED_PASSWORD_PER_PAGE * (MAX_METANAME + 1)];

    /* Placeholder for currently displayed password names, points to buffer indexes */
    const char *passwords[DISPLAYED_PASSWORD_PER_PAGE];

    /* Index of the last displayed password */
    size_t index;

    /* Display password offsets, to be able to retrieve them in metadatas */
    size_t offsets[DISPLAYED_PASSWORD_PER_PAGE];
} passwordList_t;

static passwordList_t passwordList = {.buffer = {0}, .passwords = {0}, .index = 0, .offsets = {0}};

void password_list_reset() {
    explicit_bzero(&passwordList, sizeof(passwordList));
}

void password_list_set_index(const size_t index) {
    passwordList.index = index;
}

size_t password_list_get_index() {
    return passwordList.index;
}

void password_list_incr_index() {
    passwordList.index++;
}

void password_list_set_offset(const size_t index, const size_t offset) {
    passwordList.offsets[index] = offset;
}

size_t password_list_get_offset(const size_t index) {
    return passwordList.offsets[index];
}

size_t password_list_get_current_offset() {
    return passwordList.offsets[passwordList.index];
}

void password_list_set_password(const size_t index, const char *const password) {
    passwordList.passwords[index] = password;
}

const char *password_list_get_password(const size_t index) {
    return passwordList.passwords[index];
}

char *password_list_buffer_ptr(const size_t offset) {
    return &passwordList.buffer[0] + offset;
}

void password_list_reset_buffer() {
    passwordList.buffer[0] = '\0';
}

char **password_list_passwords() {
    return (char **) passwordList.passwords;
}
