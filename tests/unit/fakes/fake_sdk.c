#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "os_io.h"
#include "os_io_legacy.h"
#include "os_nvm.h"
#include "os_utils.h"

unsigned char G_io_tx_buffer[OS_IO_BUFFER_SIZE + 1];
unsigned char G_io_seproxyhal_spi_buffer[OS_IO_SEPH_BUFFER_SIZE];

// Same semantics as the device: copy the source, or zero the destination for a NULL source
void nvm_write(void *dst_adr, void *src_adr, unsigned int src_len) {
    if (dst_adr == NULL) {
        abort();
    }
    if (src_adr == NULL) {
        memset(dst_adr, 0, src_len);
    } else {
        memmove(dst_adr, src_adr, src_len);
    }
}

// Same as the SDK: ASCII 0x20 to 0x7E, an empty string is printable
bool is_printable_string(const char *str, size_t len) {
    if (len == 0) {
        return true;
    }
    if (str == NULL) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (((unsigned char) str[i] < 0x20) || ((unsigned char) str[i] > 0x7E)) {
            return false;
        }
    }
    return true;
}
