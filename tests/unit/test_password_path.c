#include <stdarg.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "password_path.h"

// Built with -fsanitize=undefined: a signed shift overflow aborts the test.
static void test_high_bit_bytes_in_every_position(void **state __attribute__((unused))) {
    const uint8_t bytes[] = {0x80, 0xFF, 0x7F};
    for (size_t b = 0; b < sizeof(bytes); b++) {
        for (size_t pos = 0; pos < 4; pos++) {
            uint8_t digest[PASSWORD_PATH_DIGEST_SZ] = {0};
            uint32_t path[PASSWORD_PATH_LEN];
            for (size_t word = 0; word < PASSWORD_PATH_LEN - 1; word++) {
                digest[4 * word + pos] = bytes[b];
            }

            password_path_from_digest(digest, path);

            const uint32_t expected = 0x80000000u | ((uint32_t) bytes[b] << (8 * (3 - pos)));
            assert_int_equal(path[0], DERIVE_PASSWORD_PATH);
            for (size_t word = 1; word < PASSWORD_PATH_LEN; word++) {
                assert_int_equal(path[word], expected);
            }
        }
    }
}

static void test_big_endian_words(void **state __attribute__((unused))) {
    uint8_t digest[PASSWORD_PATH_DIGEST_SZ];
    uint32_t path[PASSWORD_PATH_LEN];
    for (size_t i = 0; i < sizeof(digest); i++) {
        digest[i] = 0xE0 + i;
    }

    password_path_from_digest(digest, path);

    assert_int_equal(path[0], DERIVE_PASSWORD_PATH);
    assert_int_equal(path[1], 0xE0E1E2E3);
    assert_int_equal(path[8], 0xFCFDFEFF);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_high_bit_bytes_in_every_position),
        cmocka_unit_test(test_big_endian_words),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
