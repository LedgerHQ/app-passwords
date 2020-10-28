#include "tests.h"
#include "sw.h"
#include "io.h"
#include "password_typing.h"

/* Takes a metadata as an input (charset + seed) and returns a 20 char password*/
int test_generate_password(const buf_t *input) {
    uint8_t enabledSets = input->bytes[0];
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    uint8_t *seed_ptr = input->bytes + 1;
    size_t seed_len = input->size - 1;
    type_password(seed_ptr,
                  seed_len,
                  G_io_apdu_buffer,
                  enabledSets,
                  (const uint8_t *) PIC(DEFAULT_MIN_SET),
                  20);
    const buf_t response = {.bytes = G_io_apdu_buffer, .size = 20};
    return send(&response, SW_OK);
}

int test_dispatcher(uint8_t p1, uint8_t p2, const buf_t *input) {
    switch (p1) {
        case GENERATE_PASSWORD:
            return test_generate_password(input);
        default:
            return send_sw(SW_INS_NOT_SUPPORTED + 1);
    }
}