#include "globals.h"
#include "options.h"
#include "metadata.h"
#include "password.h"
#include "password_typing.h"

error_type_t create_new_password(const char* const pwd_name, const size_t pwd_size) {
    // use the G_io_seproxyhal_spi_buffer as temp buffer to build the entry (and include the
    // requested set of chars)
    memmove(G_io_seproxyhal_spi_buffer + 1, pwd_name, pwd_size);
    // use the requested classes from the user
    G_io_seproxyhal_spi_buffer[0] = get_charset_options();
    // add the metadata
    return write_metadata(G_io_seproxyhal_spi_buffer, 1 + pwd_size);
}

void type_password_at_offset(const size_t offset) {
    unsigned char enabledSets = METADATA_SETS(offset);
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    type_password((uint8_t*) METADATA_NICKNAME(offset),
                  METADATA_NICKNAME_LEN(offset),
                  NULL,
                  enabledSets,
                  (const uint8_t*) PIC(DEFAULT_MIN_SET),
                  PASSWORD_MAX_SIZE);
}

void show_password_at_offset(const size_t offset, uint8_t* dest_buffer) {
    unsigned char enabledSets = METADATA_SETS(offset);
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    type_password((uint8_t*) METADATA_NICKNAME(offset),
                  METADATA_NICKNAME_LEN(offset),
                  dest_buffer,
                  enabledSets,
                  (const uint8_t*) PIC(DEFAULT_MIN_SET),
                  PASSWORD_MAX_SIZE);
}

error_type_t delete_password_at_offset(const size_t offset) {
    return erase_metadata(offset);
}

bool nickname_exists(const char* const pwd_name, const size_t pwd_size) {
    // write_metadata silently caps the total data block at MAX_METANAME bytes
    // (1 charset byte + nickname), so any nickname longer than MAX_METANAME - 1
    // is truncated at storage time. Mirror that truncation here, otherwise a
    // user could bypass duplicate detection just by typing a name longer than
    // what the device can actually persist.
    const size_t effective_size =
        (pwd_size > (size_t) (MAX_METANAME - 1)) ? (size_t) (MAX_METANAME - 1) : pwd_size;
    for (size_t i = 0; i < N_storage.metadata_count; i++) {
        uint32_t offset = get_metadata(i);
        if (offset == -1UL) {
            break;
        }
        if (METADATA_NICKNAME_LEN(offset) == effective_size &&
            memcmp((const void*) METADATA_NICKNAME(offset), pwd_name, effective_size) == 0) {
            return true;
        }
    }
    return false;
}
