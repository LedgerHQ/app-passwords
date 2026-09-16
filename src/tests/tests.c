#include "io.h"

#include "error.h"
#include "password.h"
#include "password_typing.h"
#include "tests.h"

// Test-only APDU handler: compiled into nothing unless built with TESTING=1,
// matching the `#ifdef TESTING` guard on the dispatch site in dispatcher.c.
#ifdef TESTING

/* Takes a metadata as an input (charset + seed) and returns a PASSWORD_MAX_SIZE char password*/
int test_generate_password(const buf_t *input) {
    // The charset byte is mandatory. Reading it from an empty payload used to fetch a byte from
    // outside the APDU data, and `input->size - 1` then underflowed the size_t to SIZE_MAX,
    // which was handed to the derivation hash as its input length.
    if ((input == NULL) || (input->size < 1)) {
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }

    uint8_t enabledSets = input->bytes[0];
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    uint8_t *seed_ptr = input->bytes + 1;
    size_t seed_len = input->size - 1;
    // generate_password() writes a NUL at out[size], so the buffer needs size + 1 bytes.
    uint8_t out_buffer[PASSWORD_MAX_SIZE + 1] = {0};
    int status = 0;

    if (!type_password(seed_ptr,
                       seed_len,
                       out_buffer,
                       enabledSets,
                       (const uint8_t *) PIC(DEFAULT_MIN_SET),
                       PASSWORD_MAX_SIZE)) {
        explicit_bzero(out_buffer, sizeof(out_buffer));
        return io_send_sw(SWO_EXECUTION_ERROR);
    }

    status = io_send_response_pointer(out_buffer, PASSWORD_MAX_SIZE, SWO_SUCCESS);
    explicit_bzero(out_buffer, sizeof(out_buffer));
    if (status > 0) {
        // API_LEVEL >= 24 status can be positive (response length) / negative (error)
        // API_LEVEL  < 24 status is 0 / -1
        status = 0;
    }
    return status;
}

int test_dispatcher(uint8_t p1, __attribute__((unused)) uint8_t p2, const buf_t *input) {
    switch (p1) {
        case GENERATE_PASSWORD:
            return test_generate_password(input);
        default:
            return io_send_sw(SWO_INVALID_INS + 1);
    }
}

#endif  // TESTING
