#include "globals.h"
#include "options.h"
#include "metadata.h"
#include "password.h"

error_type_t create_new_password(const char* const pwd_name, const size_t pwd_size) {
    // use the G_io_seproxyhal_spi_buffer as temp buffer to build the entry (and include the
    // requested set of chars)
    memmove(G_io_seproxyhal_spi_buffer + 1, pwd_name, pwd_size);
    // use the requested classes from the user
    G_io_seproxyhal_spi_buffer[0] = get_charset_options();
    // add the metadata
     return write_metadata(G_io_seproxyhal_spi_buffer, 1 + pwd_size);
}
