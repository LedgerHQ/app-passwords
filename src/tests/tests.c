#include "io.h"

#include "error.h"
#include "password.h"
#include "metadata.h"
#include "password_typing.h"
#include "tests.h"

// Test-only APDU handler: compiled into nothing unless built with TESTING=1,
// matching the `#ifdef TESTING` guard on the dispatch site in dispatcher.c.
#ifdef TESTING

/* Takes a data block as an input (see metadata.h: charset, then password length for versions
 * above 0, then seed) and the generation version, and returns the password */
int test_generate_password(uint8_t version, const buf_t *input) {
    const size_t params_len = (version == META_VERSION_0) ? 1 : 2;
    // The parameters are mandatory. Reading them from a shorter payload would fetch bytes from
    // outside the APDU data, and the seed length would underflow.
    if ((input == NULL) || (input->size < params_len)) {
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }

    uint8_t enabledSets = input->bytes[0];
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    const uint8_t length = (version == META_VERSION_0) ? PASSWORD_DEFAULT_SIZE : input->bytes[1];
    if (!password_length_is_supported(length)) {
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    uint8_t *seed_ptr = input->bytes + params_len;
    size_t seed_len = input->size - params_len;
    // generate_password() writes a NUL at out[size], so the buffer needs size + 1 bytes.
    uint8_t out_buffer[PASSWORD_MAX_SIZE + 1] = {0};
    int status = 0;

    if (!type_password(seed_ptr, seed_len, out_buffer, enabledSets, version, length)) {
        explicit_bzero(out_buffer, sizeof(out_buffer));
        return io_send_sw(SWO_EXECUTION_ERROR);
    }

    status = io_send_response_pointer(out_buffer, length, SWO_SUCCESS);
    explicit_bzero(out_buffer, sizeof(out_buffer));
    if (status > 0) {
        // API_LEVEL >= 24 status can be positive (response length) / negative (error)
        // API_LEVEL  < 24 status is 0 / -1
        status = 0;
    }
    return status;
}

int test_dispatcher(uint8_t p1, uint8_t p2, const buf_t *input) {
    switch (p1) {
        case GENERATE_PASSWORD:
            // P2 is the generation version.
            return test_generate_password(p2, input);
        default:
            return io_send_sw(SWO_INVALID_INS + 1);
    }
}

#endif  // TESTING
