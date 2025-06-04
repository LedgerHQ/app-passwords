#include <io.h>

#include "error.h"
#include "password_typing.h"
#include "tests.h"

/* Takes a metadata as an input (charset + seed) and returns a 20 char password*/
int test_generate_password(const buf_t *input) {
    uint8_t enabledSets = input->bytes[0];
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    uint8_t *seed_ptr = input->bytes + 1;
    size_t seed_len = input->size - 1;
    uint8_t out_buffer[20];
    type_password(seed_ptr,
                  seed_len,
                  out_buffer,
                  enabledSets,
                  (const uint8_t *) PIC(DEFAULT_MIN_SET),
                  sizeof(out_buffer));
    return io_send_response_pointer(out_buffer, 20, SW_OK);
}

int test_dispatcher(uint8_t p1, __attribute__((unused)) uint8_t p2, const buf_t *input) {
    switch (p1) {
        case GENERATE_PASSWORD:
            return test_generate_password(input);
        default:
            return io_send_sw(SW_INS_NOT_SUPPORTED + 1);
    }
}
