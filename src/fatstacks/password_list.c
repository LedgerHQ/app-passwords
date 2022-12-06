#if defined(TEST)
#include <bsd/string.h>
#else
#include <string.h>
#endif

#include "password_list.h"

typedef struct passwordList_s {
    /*
     * Buffer where password names are stored
     */
    char buffer[DISPLAYED_PASSWORD_PER_PAGE * (MAX_METANAME + 1)];
    /*
     * Used to keep track of password store in buffer. Start at 0, is incremented with password
     * length (including trailing '\0' each time a password is stored.
     */
    size_t currentOffset;
    /*
     * Placeholder for currently displayed password names, points to previous `buffer` locations.
     * Also used to display currently selected password name (for display or deletion)
     */
    const char *passwords[DISPLAYED_PASSWORD_PER_PAGE];
    /*
     * Like `passwords` keeps a relation between currently displayed password indexes and their
     * names, this array keeps the relation between currently displayed password indexes and their
     * offset in metadatas.
     */
    size_t offsets[DISPLAYED_PASSWORD_PER_PAGE];
    /*
     * Used to pin an index in order to retrieve the related offset with
     * `password_list_get_current_offset`. Needed as the offset is used in a callback function with
     * no meaningful arguments.
     */
    size_t index;
} passwordList_t;

static passwordList_t passwordList = {.buffer = {0},
                                      .currentOffset = 0,
                                      .passwords = {0},
                                      .offsets = {0},
                                      .index = 0};

void password_list_reset() {
    explicit_bzero(&passwordList, sizeof(passwordList));
}

size_t password_list_get_offset(const size_t index) {
    if (index >= DISPLAYED_PASSWORD_PER_PAGE) {
        return -1;
    }
    return passwordList.offsets[index];
}

size_t password_list_get_current_offset() {
    return passwordList.offsets[passwordList.index];
}

const char *password_list_get_password(const size_t index) {
    if (index >= DISPLAYED_PASSWORD_PER_PAGE) {
        return NULL;
    }
    return passwordList.passwords[index];
}

void password_list_set_current(const size_t index) {
    passwordList.index = index;
}

bool password_list_add_password(const size_t index,
                                const size_t offset,
                                const char *const password,
                                const size_t length) {
    if (index >= DISPLAYED_PASSWORD_PER_PAGE) {
        return false;
    }
    passwordList.offsets[index] = offset;
    void *nextPwdPtr = &passwordList.buffer[0] + passwordList.currentOffset;
    strlcpy(nextPwdPtr, password, length);
    passwordList.passwords[index] = nextPwdPtr;
    passwordList.currentOffset += length;
    return true;
}

void password_list_reset_buffer() {
    explicit_bzero(&passwordList.buffer[0], sizeof(passwordList.buffer));
    passwordList.currentOffset = 0;
}

char **password_list_passwords() {
    return (char **) passwordList.passwords;
}
