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

error_type_t reset_password_at_offset(const size_t offset) {
    return erase_metadata(offset);
}
