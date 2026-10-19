#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "ctaes.h"
#include "ctr_drbg.h"
#include "metadata.h"
#include "password_generation.h"

// DRBG seeds the device derives for these nicknames from the Speculos default seed, so the
// expected passwords are the ones of the functional tests (tests/functional/tests_vectors.py)
static const uint8_t SEED_GMAIL[32] = {
    0x02, 0xbb, 0x96, 0x5b, 0x1d, 0x7b, 0x63, 0xe6, 0x44, 0x15, 0x43, 0xbb, 0xc3, 0xdf, 0x87, 0xfc,
    0x73, 0x58, 0x29, 0xc1, 0x7f, 0x04, 0x51, 0x38, 0x9f, 0xd6, 0xe9, 0x3e, 0xc4, 0x82, 0xc8, 0x67};
static const uint8_t SEED_BANK[32] = {
    0x84, 0x69, 0x41, 0xf1, 0x54, 0xe3, 0xa4, 0x08, 0x60, 0xfe, 0x31, 0x4d, 0x70, 0xcf, 0xff, 0x1f,
    0xdd, 0xd4, 0x69, 0x3e, 0x9b, 0x6c, 0x68, 0xa4, 0x5b, 0x4d, 0xe8, 0x5b, 0x24, 0x75, 0xbf, 0xf6};
static const uint8_t SEED_LOWER_20[32] = {
    0x2e, 0xd5, 0x52, 0x99, 0x07, 0xe9, 0x4c, 0x6c, 0x47, 0x90, 0xf1, 0xc5, 0xb3, 0x69, 0x5f, 0x7f,
    0x71, 0x6e, 0xc4, 0x3f, 0xd1, 0xaa, 0x2e, 0xd6, 0xe8, 0x1d, 0x7e, 0xd2, 0x75, 0x12, 0x8f, 0xf9};
static const uint8_t SEED_MIXED_20[32] = {
    0xa0, 0x63, 0x24, 0x75, 0x53, 0xe7, 0x89, 0xe3, 0x02, 0xc0, 0x3b, 0xe7, 0x0c, 0xa1, 0xc7, 0xa5,
    0x45, 0x38, 0x11, 0x04, 0x1d, 0xe3, 0x1f, 0xef, 0x24, 0xdb, 0xfc, 0xdf, 0xdd, 0x32, 0x2f, 0xba};

typedef struct {
    uint8_t sets;
    const uint8_t *seed;
    uint8_t length;
    const char *expected;
} vector_t;

static const char *const SETS[NUM_SETS] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                                           "abcdefghijklmnopqrstuvwxyz",
                                           "0123456789",
                                           "-",
                                           "_",
                                           " ",
                                           "\"#$%&'*+,./:;=?!@\\^`|~",
                                           "[]{}()<>"};

typedef struct {
    const uint8_t *data;
    size_t len;
    size_t offset;
    bool fail;
} entropy_t;

// Hands out consecutive bytes of the entropy buffer, as the device hands out its seed
static int entropy_cb(void *context, unsigned char *buffer, size_t size) {
    entropy_t *entropy = (entropy_t *) context;
    if (entropy->fail || (size > entropy->len - entropy->offset)) {
        return 1;
    }
    memcpy(buffer, entropy->data + entropy->offset, size);
    entropy->offset += size;
    return 0;
}

static mbedtls_ctr_drbg_context drbg;
static entropy_t entropy;

// Seeds the DRBG as type_password() does: a 32-byte seed, no personalization string
static void seed_drbg(const uint8_t seed[32]) {
    entropy = (entropy_t) {.data = seed, .len = 32};
    mbedtls_ctr_drbg_init(&drbg);
    TEST_ASSERT_EQUAL(0, mbedtls_ctr_drbg_seed(&drbg, entropy_cb, &entropy, NULL, 0));
}

static void check_vectors(const vector_t *vectors, size_t count, uint8_t version) {
    for (size_t i = 0; i < count; i++) {
        uint8_t out[PASSWORD_MAX_SIZE + 1];
        memset(out, 0xAA, sizeof(out));
        seed_drbg(vectors[i].seed);
        TEST_ASSERT_TRUE(
            generate_password(&drbg, vectors[i].sets, version, out, vectors[i].length));
        TEST_ASSERT_EQUAL_STRING(vectors[i].expected, (const char *) out);
        mbedtls_ctr_drbg_free(&drbg);
    }
}

// True if the password holds a character of one of the sets of `sets`
static bool has_char_of(const uint8_t *password, uint8_t sets) {
    for (size_t s = 0; s < NUM_SETS; s++) {
        if ((sets & (1 << s)) && (strpbrk((const char *) password, SETS[s]) != NULL)) {
            return true;
        }
    }
    return false;
}

void setUp(void) {
}

void tearDown(void) {
    mbedtls_ctr_drbg_free(&drbg);
}

/* ----------------------------------------------------------------------- */
/* Primitives                                                              */
/* ----------------------------------------------------------------------- */

// FIPS-197, appendix C.3
static void test_aes256_known_answer(void) {
    uint8_t key[32];
    for (size_t i = 0; i < sizeof(key); i++) {
        key[i] = i;
    }
    const uint8_t plain[16] = {0x00,
                               0x11,
                               0x22,
                               0x33,
                               0x44,
                               0x55,
                               0x66,
                               0x77,
                               0x88,
                               0x99,
                               0xaa,
                               0xbb,
                               0xcc,
                               0xdd,
                               0xee,
                               0xff};
    const uint8_t cipher[16] = {0x8e,
                                0xa2,
                                0xb7,
                                0xca,
                                0x51,
                                0x67,
                                0x45,
                                0xbf,
                                0xea,
                                0xfc,
                                0x49,
                                0x90,
                                0x4b,
                                0x49,
                                0x60,
                                0x89};
    AES256_ctx ctx;
    uint8_t out[16];

    AES256_init(&ctx, key);
    AES256_encrypt(&ctx, 1, out, plain);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(cipher, out, sizeof(out));
    AES256_decrypt(&ctx, 1, out, cipher);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(plain, out, sizeof(out));
}

// NIST CTR_DRBG AES-256 with derivation function, prediction resistance off
static void test_ctr_drbg_known_answer(void) {
    static const uint8_t source[64] = {
        0x5a, 0x19, 0x4d, 0x5e, 0x2b, 0x31, 0x58, 0x14, 0x54, 0xde, 0xf6, 0x75, 0xfb,
        0x79, 0x58, 0xfe, 0xc7, 0xdb, 0x87, 0x3e, 0x56, 0x89, 0xfc, 0x9d, 0x03, 0x21,
        0x7c, 0x68, 0xd8, 0x03, 0x38, 0x20, 0xf9, 0xe6, 0x5e, 0x04, 0xd8, 0x56, 0xf3,
        0xa9, 0xc4, 0x4a, 0x4c, 0xbd, 0xc1, 0xd0, 0x08, 0x46, 0xf5, 0x98, 0x3d, 0x77,
        0x1c, 0x1b, 0x13, 0x7e, 0x4e, 0x0f, 0x9d, 0x8e, 0xf4, 0x09, 0xf9, 0x2e};
    static const uint8_t nonce[16] = {0x1b,
                                      0x54,
                                      0xb8,
                                      0xff,
                                      0x06,
                                      0x42,
                                      0xbf,
                                      0xf5,
                                      0x21,
                                      0xf1,
                                      0x5c,
                                      0x1c,
                                      0x0b,
                                      0x66,
                                      0x5f,
                                      0x3f};
    static const uint8_t expected[16] = {0xa0,
                                         0x54,
                                         0x30,
                                         0x3d,
                                         0x8a,
                                         0x7e,
                                         0xa9,
                                         0x88,
                                         0x9d,
                                         0x90,
                                         0x3e,
                                         0x07,
                                         0x7c,
                                         0x6f,
                                         0x21,
                                         0x8f};
    uint8_t out[16];

    entropy = (entropy_t) {.data = source, .len = sizeof(source)};
    mbedtls_ctr_drbg_init(&drbg);
    TEST_ASSERT_EQUAL(
        0,
        mbedtls_ctr_drbg_seed_entropy_len(&drbg, entropy_cb, &entropy, nonce, 16, 32));
    TEST_ASSERT_EQUAL(0, mbedtls_ctr_drbg_random(&drbg, out, sizeof(out)));
    TEST_ASSERT_EQUAL(0, mbedtls_ctr_drbg_reseed(&drbg, NULL, 0));
    TEST_ASSERT_EQUAL(0, mbedtls_ctr_drbg_random(&drbg, out, sizeof(out)));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, sizeof(out));
}

// NIST CTR_DRBG AES-256 with derivation function, prediction resistance on
static void test_ctr_drbg_known_answer_prediction_resistance(void) {
    static const uint8_t source[96] = {
        0xc1, 0x80, 0x81, 0xa6, 0x5d, 0x44, 0x02, 0x16, 0x19, 0xb3, 0xf1, 0x80, 0xb1, 0xc9,
        0x20, 0x02, 0x6a, 0x54, 0x6f, 0x0c, 0x70, 0x81, 0x49, 0x8b, 0x6e, 0xa6, 0x62, 0x52,
        0x6d, 0x51, 0xb1, 0xcb, 0x58, 0x3b, 0xfa, 0xd5, 0x37, 0x5f, 0xfb, 0xc9, 0xff, 0x46,
        0xd2, 0x19, 0xc7, 0x22, 0x3e, 0x95, 0x45, 0x9d, 0x82, 0xe1, 0xe7, 0x22, 0x9f, 0x63,
        0x31, 0x69, 0xd2, 0x6b, 0x57, 0x47, 0x4f, 0xa3, 0x37, 0xc9, 0x98, 0x1c, 0x0b, 0xfb,
        0x91, 0x31, 0x4d, 0x55, 0xb9, 0xe9, 0x1c, 0x5a, 0x5e, 0xe4, 0x93, 0x92, 0xcf, 0xc5,
        0x23, 0x12, 0xd5, 0x56, 0x2c, 0x4a, 0x6e, 0xff, 0xdc, 0x10, 0xd0, 0x68};
    static const uint8_t nonce[16] = {0xd2,
                                      0x54,
                                      0xfc,
                                      0xff,
                                      0x02,
                                      0x1e,
                                      0x69,
                                      0xd2,
                                      0x29,
                                      0xc9,
                                      0xcf,
                                      0xad,
                                      0x85,
                                      0xfa,
                                      0x48,
                                      0x6c};
    static const uint8_t expected[16] = {0x34,
                                         0x01,
                                         0x16,
                                         0x56,
                                         0xb4,
                                         0x29,
                                         0x00,
                                         0x8f,
                                         0x35,
                                         0x63,
                                         0xec,
                                         0xb5,
                                         0xf2,
                                         0x59,
                                         0x07,
                                         0x23};
    uint8_t out[16];

    entropy = (entropy_t) {.data = source, .len = sizeof(source)};
    mbedtls_ctr_drbg_init(&drbg);
    TEST_ASSERT_EQUAL(
        0,
        mbedtls_ctr_drbg_seed_entropy_len(&drbg, entropy_cb, &entropy, nonce, 16, 32));
    mbedtls_ctr_drbg_set_prediction_resistance(&drbg, MBEDTLS_CTR_DRBG_PR_ON);
    TEST_ASSERT_EQUAL(0, mbedtls_ctr_drbg_random(&drbg, out, sizeof(out)));
    TEST_ASSERT_EQUAL(0, mbedtls_ctr_drbg_random(&drbg, out, sizeof(out)));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, sizeof(out));
}

static void test_ctr_drbg_entropy_failure(void) {
    entropy = (entropy_t) {.data = SEED_GMAIL, .len = 32, .fail = true};
    mbedtls_ctr_drbg_init(&drbg);
    TEST_ASSERT_EQUAL(MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED,
                      mbedtls_ctr_drbg_seed(&drbg, entropy_cb, &entropy, NULL, 0));
}

static void test_ctr_drbg_request_too_big(void) {
    static uint8_t out[MBEDTLS_CTR_DRBG_MAX_REQUEST + 1];

    seed_drbg(SEED_GMAIL);
    TEST_ASSERT_EQUAL(MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG,
                      mbedtls_ctr_drbg_random(&drbg, out, sizeof(out)));
    TEST_ASSERT_EQUAL(0, mbedtls_ctr_drbg_random(&drbg, out, sizeof(out) - 1));
}

/* ----------------------------------------------------------------------- */
/* Functional test vectors                                                 */
/* ----------------------------------------------------------------------- */

static void test_generate_password_v0_vectors(void) {
    static const vector_t vectors[] = {
        {0x01, SEED_GMAIL, 20, "HMYDQUIOVKPCKJIHQJEN"},
        {0x03, SEED_GMAIL, 20, "KqIJcPjhENivHvOdmuKQ"},
        {0x07, SEED_GMAIL, 20, "xNX8IQO4vP0ucO41J6JW"},
        {0x0F, SEED_GMAIL, 20, "w14JrbA9HNvWU1ON5MGP"},
        {0x1F, SEED_GMAIL, 20, "vy4Joa86FKvVS1ON4KEP"},
        {0x3F, SEED_GMAIL, 20, "kD83CP1UZO vQvJIuNx4"},
        {0x7F, SEED_GMAIL, 20, "?u8htP1|DO v7vJzYNb4"},
        {0xFF, SEED_GMAIL, 20, "*m8ZlP1|}O vzvJrQNT4"},
        {0xFF, SEED_LOWER_20, 20, "29!uO;UPx UT8Hkmi- 5"},
        {0xFF, SEED_MIXED_20, 20, " $4,P.usI*C\\k1fv2;M;"},
    };
    check_vectors(vectors, ARRAYLEN(vectors), META_VERSION_0);
}

static void test_generate_password_v1_vectors(void) {
    static const vector_t vectors[] = {
        {0x0F, SEED_GMAIL, 20, "-5r4GUPb19HMNwvNJW8O"},
        {0x3F, SEED_GMAIL, 20, "kD83CP1UZO vQvJIuNx4"},
        {0x7F, SEED_GMAIL, 20, "?u8htP1|DO v7vJ*YNb4"},
        {0xC7, SEED_GMAIL, 20, "Oc4Pv]v1^dJ<2Wo;8NpT"},
        {0xFF, SEED_GMAIL, 20, "*m8ZlP1|}O vzvJ^QNT4"},
        {0xFF, SEED_LOWER_20, 20, "29!uO;UPx UT8Hkmi$-5"},
        {0xFF, SEED_MIXED_20, 20, "1fs,2$u4|;k;v*IM_C\\P"},
        {0x04, SEED_BANK, 4, "2968"},
        {0x04, SEED_BANK, 6, "685962"},
        {0x07, SEED_GMAIL, 8, "O14WvxJP"},
        {0xFF, SEED_GMAIL, 12, "m 4zvPQ}^ZJ1"},
        {0xFF, SEED_GMAIL, 32, " F4Os2!Nvvd`^P(lO8P1}|QzZJTm*/(a"},
        {0xFF, SEED_GMAIL, 64, "h5ZXN1s`|FW^2ATL_PvwO1a/*J8Zk\"b}PT)z}~ *!v9Qy((H+14?dTuOlm7{>=1F"},
    };
    check_vectors(vectors, ARRAYLEN(vectors), META_VERSION_1);
}

/* ----------------------------------------------------------------------- */
/* Properties                                                              */
/* ----------------------------------------------------------------------- */

// Every accepted charset: only enabled characters, and one of each enabled group
static void test_generate_password_v1_draws_every_group(void) {
    static const uint8_t groups[] = {UPPERCASE,
                                     LOWERCASE,
                                     NUMBERS,
                                     MINUS | UNDERLINE | SPACE,
                                     SPECIAL | BRACKETS};
    for (unsigned int sets = 1; sets <= 0xFF; sets++) {
        if (charsets_are_weak(sets)) {
            continue;
        }
        uint8_t out[PASSWORD_DEFAULT_SIZE + 1];
        seed_drbg(SEED_GMAIL);
        TEST_ASSERT_TRUE(generate_password(&drbg, sets, META_VERSION_1, out, sizeof(out) - 1));
        TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, strlen((const char *) out));
        for (size_t i = 0; i < PASSWORD_DEFAULT_SIZE; i++) {
            const char c[2] = {out[i], '\0'};
            TEST_ASSERT_TRUE(has_char_of((const uint8_t *) c, sets));
        }
        for (size_t g = 0; g < sizeof(groups); g++) {
            if (sets & groups[g]) {
                TEST_ASSERT_TRUE(has_char_of(out, sets & groups[g]));
            }
        }
        mbedtls_ctr_drbg_free(&drbg);
    }
}

static void test_generate_password_refusals(void) {
    uint8_t out[PASSWORD_MAX_SIZE + 1];

    seed_drbg(SEED_GMAIL);
    // Unknown version
    TEST_ASSERT_FALSE(generate_password(&drbg, 0xFF, META_CURRENT_VERSION + 1, out, 20));
    // No set at all
    TEST_ASSERT_FALSE(generate_password(&drbg, 0x00, META_VERSION_0, out, 20));
    // Separators only, for a new entry
    TEST_ASSERT_FALSE(generate_password(&drbg, 0x38, META_VERSION_1, out, 20));
    // PIN length with letters
    TEST_ASSERT_FALSE(generate_password(&drbg, 0x07, META_VERSION_1, out, 4));
    // Fewer characters than the groups to draw from
    TEST_ASSERT_FALSE(generate_password(&drbg, 0xFF, META_VERSION_0, out, 3));
}

// Separators only stay allowed for version 0, which existing entries may use
static void test_generate_password_v0_separators_only(void) {
    uint8_t out[PASSWORD_DEFAULT_SIZE + 1];

    seed_drbg(SEED_GMAIL);
    TEST_ASSERT_TRUE(generate_password(&drbg, 0x38, META_VERSION_0, out, PASSWORD_DEFAULT_SIZE));
    TEST_ASSERT_EQUAL(strspn((const char *) out, "-_ "), PASSWORD_DEFAULT_SIZE);
}

static void test_generate_password_drbg_failure(void) {
    uint8_t out[PASSWORD_DEFAULT_SIZE + 1];

    // The seed is used up: the next reseed, forced on every draw, fails
    seed_drbg(SEED_GMAIL);
    mbedtls_ctr_drbg_set_prediction_resistance(&drbg, MBEDTLS_CTR_DRBG_PR_ON);
    TEST_ASSERT_FALSE(generate_password(&drbg, 0xFF, META_VERSION_1, out, PASSWORD_DEFAULT_SIZE));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_aes256_known_answer);
    RUN_TEST(test_ctr_drbg_known_answer);
    RUN_TEST(test_ctr_drbg_known_answer_prediction_resistance);
    RUN_TEST(test_ctr_drbg_entropy_failure);
    RUN_TEST(test_ctr_drbg_request_too_big);
    RUN_TEST(test_generate_password_v0_vectors);
    RUN_TEST(test_generate_password_v1_vectors);
    RUN_TEST(test_generate_password_v1_draws_every_group);
    RUN_TEST(test_generate_password_refusals);
    RUN_TEST(test_generate_password_v0_separators_only);
    RUN_TEST(test_generate_password_drbg_failure);
    return UNITY_END();
}
