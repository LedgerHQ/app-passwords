#include "globals.h"
#include "options.h"
#include "metadata.h"
#include "password.h"
#include "password_typing.h"

error_type_t create_new_password(const char *const pwd_name, const size_t pwd_size) {
    // write_metadata() clamps the data block to MAX_METANAME, which would drop the tail of the
    // nickname without telling anyone. Refuse the entry instead, so a caller can never persist
    // something different from what it was given.
    if (pwd_size > MAX_NICKNAME_LEN) {
        return ERR_METADATA_ENTRY_TOO_BIG;
    }
    // use the G_io_seproxyhal_spi_buffer as temp buffer to build the entry (and include the
    // requested set of chars)
    memmove(G_io_seproxyhal_spi_buffer + 1, pwd_name, pwd_size);
    // use the requested classes from the user
    G_io_seproxyhal_spi_buffer[0] = get_charset_options();
    // add the metadata
    return write_metadata(G_io_seproxyhal_spi_buffer, 1 + pwd_size);
}

bool type_password_at_offset(const size_t offset) {
    unsigned char enabledSets = METADATA_SETS(offset);
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    return type_password((uint8_t *) METADATA_NICKNAME(offset),
                         METADATA_NICKNAME_LEN(offset),
                         NULL,
                         enabledSets,
                         (const uint8_t *) PIC(DEFAULT_MIN_SET),
                         PASSWORD_MAX_SIZE);
}

bool show_password_at_offset(const size_t offset, uint8_t *dest_buffer) {
    unsigned char enabledSets = METADATA_SETS(offset);
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    return type_password((uint8_t *) METADATA_NICKNAME(offset),
                         METADATA_NICKNAME_LEN(offset),
                         dest_buffer,
                         enabledSets,
                         (const uint8_t *) PIC(DEFAULT_MIN_SET),
                         PASSWORD_MAX_SIZE);
}

error_type_t delete_password_at_offset(const size_t offset) {
    return erase_metadata(offset);
}

bool nickname_exists(const char *const pwd_name, const size_t pwd_size) {
    // Entries stored before create_new_password() started refusing over-long nicknames were
    // truncated to MAX_NICKNAME_LEN by write_metadata(). Mirror that truncation here, otherwise
    // a longer input would not be recognised as a duplicate of one of them.
    const size_t effective_size =
        (pwd_size > (size_t) MAX_NICKNAME_LEN) ? (size_t) MAX_NICKNAME_LEN : pwd_size;
    for (size_t i = 0; i < N_storage.metadata_count; i++) {
        uint32_t offset = get_metadata(i);
        if (offset == UINT32_MAX) {
            break;
        }
        if (METADATA_NICKNAME_LEN(offset) == effective_size &&
            memcmp((const void *) METADATA_NICKNAME(offset), pwd_name, effective_size) == 0) {
            return true;
        }
    }
    return false;
}
