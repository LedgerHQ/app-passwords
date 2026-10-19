#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "password_path.h"

void setUp(void) {
}

void tearDown(void) {
}

// Built with -fsanitize=undefined: a signed shift overflow aborts the test.
static void test_high_bit_bytes_in_every_position(void) {
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
            TEST_ASSERT_EQUAL_HEX32(DERIVE_PASSWORD_PATH, path[0]);
            for (size_t word = 1; word < PASSWORD_PATH_LEN; word++) {
                TEST_ASSERT_EQUAL_HEX32(expected, path[word]);
            }
        }
    }
}

static void test_big_endian_words(void) {
    uint8_t digest[PASSWORD_PATH_DIGEST_SZ];
    uint32_t path[PASSWORD_PATH_LEN];
    for (size_t i = 0; i < sizeof(digest); i++) {
        digest[i] = 0xE0 + i;
    }

    password_path_from_digest(digest, path);

    TEST_ASSERT_EQUAL_HEX32(DERIVE_PASSWORD_PATH, path[0]);
    TEST_ASSERT_EQUAL_HEX32(0xE0E1E2E3, path[1]);
    TEST_ASSERT_EQUAL_HEX32(0xFCFDFEFF, path[8]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_high_bit_bytes_in_every_position);
    RUN_TEST(test_big_endian_words);
    return UNITY_END();
}
