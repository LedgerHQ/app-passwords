#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "exceptions.h"
#include "os_seed.h"
#include "hid_mapping.h"
#include "metadata.h"
#include "password_path.h"
#include "password_typing.h"
#include "fake_vars.h"
#include "Mocklcx_sha256.h"
#include "Mockos_io_seph_cmd.h"
#include "Mockusbd_ledger.h"

// Hashes and derivation of the nickname "gmail" from the Speculos default seed, so the expected
// passwords are the ones of the functional tests (tests/functional/tests_vectors.py)
static const uint8_t GMAIL_DIGEST[32] = {
    0x57, 0x6b, 0xa7, 0xc2, 0xe4, 0xab, 0xb7, 0x18, 0x4c, 0xa4, 0x09, 0x15, 0x4d, 0xbb, 0xbd, 0x53,
    0x06, 0xc1, 0xa8, 0x07, 0x47, 0xfb, 0xce, 0x41, 0x48, 0xea, 0x62, 0x71, 0xfd, 0x21, 0xe7, 0x76};
static const uint8_t GMAIL_KEY_CHAIN[64] = {
    0x3e, 0x71, 0x39, 0x67, 0xbb, 0x02, 0x58, 0xe1, 0x9b, 0x03, 0x1a, 0x23, 0xb4, 0x6d, 0x40, 0x40,
    0xa5, 0x16, 0x62, 0x43, 0x30, 0xd9, 0xe7, 0x15, 0xbd, 0x12, 0x3b, 0xfa, 0x58, 0x7e, 0x77, 0x32,
    0x59, 0x95, 0xe7, 0x28, 0x4b, 0xd6, 0x95, 0x95, 0x85, 0xfa, 0x68, 0xb1, 0x35, 0xd0, 0x29, 0xe4,
    0xd8, 0x1b, 0xde, 0xe0, 0xad, 0xfa, 0x00, 0xf9, 0x36, 0x80, 0xc7, 0xef, 0x3b, 0x37, 0x43, 0xd0};
static const uint8_t GMAIL_DRBG_SEED[32] = {
    0x02, 0xbb, 0x96, 0x5b, 0x1d, 0x7b, 0x63, 0xe6, 0x44, 0x15, 0x43, 0xbb, 0xc3, 0xdf, 0x87, 0xfc,
    0x73, 0x58, 0x29, 0xc1, 0x7f, 0x04, 0x51, 0x38, 0x9f, 0xd6, 0xe9, 0x3e, 0xc4, 0x82, 0xc8, 0x67};

#define REPORT_SIZE 8
static const uint8_t EMPTY_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SPACE_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_REPORT[REPORT_SIZE] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_LOCK_REPORT[REPORT_SIZE] =
    {0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t ENTER_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00};

#define MAX_REPORTS 512
static uint8_t reports[MAX_REPORTS][REPORT_SIZE];
static size_t report_count;

static size_t hash_calls;
static size_t derive_calls;
// Number of the hash call (from 1) to fail, 0 for none
static size_t failing_hash;
static bool failing_derive;

static size_t fake_cx_hash_sha256(const uint8_t *in,
                                  size_t len,
                                  uint8_t *out,
                                  size_t out_len,
                                  int cmock_num_calls) {
    (void) cmock_num_calls;
    hash_calls++;
    TEST_ASSERT_TRUE(out_len >= 32);
    if (hash_calls == failing_hash) {
        return 0;
    }
    if ((len == strlen("gmail")) && (memcmp(in, "gmail", len) == 0)) {
        memcpy(out, GMAIL_DIGEST, 32);
    } else if ((len == sizeof(GMAIL_KEY_CHAIN)) && (memcmp(in, GMAIL_KEY_CHAIN, len) == 0)) {
        memcpy(out, GMAIL_DRBG_SEED, 32);
    } else {
        TEST_FAIL_MESSAGE("Unexpected data hashed");
    }
    return 32;
}

// Not a CMock mock: CMock does not parse the PLENGTH() annotations of os_seed.h
void os_perso_derive_node_with_seed_key(unsigned int mode,
                                        cx_curve_t curve,
                                        const unsigned int *path,
                                        unsigned int pathLength,
                                        unsigned char *privateKey,
                                        unsigned char *chain,
                                        unsigned char *seed_key,
                                        unsigned int seed_key_length) {
    uint32_t expected_path[PASSWORD_PATH_LEN];

    derive_calls++;
    TEST_ASSERT_EQUAL(HDW_NORMAL, mode);
    TEST_ASSERT_EQUAL(CX_CURVE_SECP256K1, curve);
    TEST_ASSERT_NULL(seed_key);
    TEST_ASSERT_EQUAL(0, seed_key_length);
    password_path_from_digest(GMAIL_DIGEST, expected_path);
    TEST_ASSERT_EQUAL(PASSWORD_PATH_LEN, pathLength);
    TEST_ASSERT_EQUAL_HEX32_ARRAY(expected_path, path, PASSWORD_PATH_LEN);
    if (failing_derive) {
        THROW(EXCEPTION);
    }
    memcpy(privateKey, GMAIL_KEY_CHAIN, 32);
    memcpy(chain, GMAIL_KEY_CHAIN + 32, 32);
}

static uint32_t fake_USBD_LEDGER_send(uint8_t class_type,
                                      uint8_t packet_type,
                                      const uint8_t *packet,
                                      uint16_t packet_length,
                                      uint32_t timeout_ms,
                                      int cmock_num_calls) {
    (void) packet_type;
    (void) timeout_ms;
    (void) cmock_num_calls;
    TEST_ASSERT_EQUAL(USBD_LEDGER_CLASS_HID_KBD, class_type);
    TEST_ASSERT_EQUAL(REPORT_SIZE, packet_length);
    TEST_ASSERT_TRUE(report_count < MAX_REPORTS);
    memcpy(reports[report_count++], packet, REPORT_SIZE);
    return 0;
}

/* Reports expected for typing `password`: a shift press and release first, then each key
 * followed by its release, a space after a dead key, and the Caps Lock and Enter keys */
static size_t expected_reports(const char *password,
                               bool caps_lock,
                               bool enter,
                               uint8_t expected[][REPORT_SIZE]) {
    size_t n = 0;
#define PUSH(r) memcpy(expected[n++], (r), REPORT_SIZE)
    PUSH(EMPTY_REPORT);
    PUSH(CAPS_REPORT);
    PUSH(EMPTY_REPORT);
    if (caps_lock) {
        PUSH(CAPS_LOCK_REPORT);
        PUSH(EMPTY_REPORT);
    }
    for (size_t i = 0; password[i] != '\0'; i++) {
        uint8_t key[REPORT_SIZE] = {0};
        TEST_ASSERT_TRUE(map_char(N_storage_rw.keyboard_layout, password[i], key));
        PUSH(key);
        PUSH((key[0] & SHIFT_KEY) ? CAPS_REPORT : EMPTY_REPORT);
        if (is_dead_key(N_storage_rw.keyboard_layout, password[i])) {
            PUSH(SPACE_REPORT);
            PUSH(EMPTY_REPORT);
        }
    }
    PUSH(EMPTY_REPORT);
    if (caps_lock) {
        PUSH(CAPS_LOCK_REPORT);
        PUSH(EMPTY_REPORT);
    }
    if (enter) {
        PUSH(ENTER_REPORT);
        PUSH(EMPTY_REPORT);
    }
#undef PUSH
    return n;
}

static void check_typed(const char *password, bool caps_lock, bool enter) {
    static uint8_t expected[MAX_REPORTS][REPORT_SIZE];
    const size_t count = expected_reports(password, caps_lock, enter, expected);

    TEST_ASSERT_EQUAL(count, report_count);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, reports, count * REPORT_SIZE);
}

static bool type_gmail(uint8_t *out, uint8_t sets, uint8_t version, uint32_t size) {
    return type_password((uint8_t *) "gmail", strlen("gmail"), out, sets, version, size);
}

void setUp(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
    N_storage_rw.keyboard_layout = HID_MAPPING_QWERTY;
    G_led_status = 0;
    memset(reports, 0, sizeof(reports));
    report_count = 0;
    hash_calls = 0;
    derive_calls = 0;
    failing_hash = 0;
    failing_derive = false;
    Mocklcx_sha256_Init();
    Mockos_io_seph_cmd_Init();
    Mockusbd_ledger_Init();
    cx_hash_sha256_StubWithCallback(fake_cx_hash_sha256);
    USBD_LEDGER_send_StubWithCallback(fake_USBD_LEDGER_send);
    os_io_seph_cmd_general_status_IgnoreAndReturn(0);
}

void tearDown(void) {
    Mocklcx_sha256_Verify();
    Mockos_io_seph_cmd_Verify();
    Mockusbd_ledger_Verify();
    Mocklcx_sha256_Destroy();
    Mockos_io_seph_cmd_Destroy();
    Mockusbd_ledger_Destroy();
}

/* ----------------------------------------------------------------------- */
/* Displayed password                                                      */
/* ----------------------------------------------------------------------- */

static void test_show_password(void) {
    uint8_t out[PASSWORD_MAX_SIZE + 1];

    TEST_ASSERT_TRUE(type_gmail(out, 0x07, META_VERSION_0, 20));
    TEST_ASSERT_EQUAL_STRING("xNX8IQO4vP0ucO41J6JW", (const char *) out);
    TEST_ASSERT_TRUE(type_gmail(out, 0xFF, META_VERSION_1, 20));
    TEST_ASSERT_EQUAL_STRING("*m8ZlP1|}O vzvJ^QNT4", (const char *) out);
    TEST_ASSERT_TRUE(type_gmail(out, 0xFF, META_VERSION_1, 64));
    TEST_ASSERT_EQUAL_STRING("h5ZXN1s`|FW^2ATL_PvwO1a/*J8Zk\"b}PT)z}~ *!v9Qy((H+14?dTuOlm7{>=1F",
                             (const char *) out);
    // Nothing is typed when the password is displayed
    TEST_ASSERT_EQUAL(0, report_count);
}

static void test_show_password_wipes_a_failed_generation(void) {
    uint8_t out[PASSWORD_MAX_SIZE + 1];
    memset(out, 0xAA, sizeof(out));

    // Separators only are refused for a new entry, once the DRBG is seeded
    TEST_ASSERT_FALSE(type_gmail(out, 0x38, META_VERSION_1, 20));
    TEST_ASSERT_EQUAL(2, hash_calls);
    for (size_t i = 0; i <= 20; i++) {
        TEST_ASSERT_EQUAL_HEX8(0, out[i]);
    }
    TEST_ASSERT_EQUAL_HEX8(0xAA, out[21]);
}

/* ----------------------------------------------------------------------- */
/* Failures                                                                */
/* ----------------------------------------------------------------------- */

static void test_type_password_rejects_the_size(void) {
    uint8_t out[PASSWORD_MAX_SIZE + 2];

    TEST_ASSERT_FALSE(type_gmail(out, 0xFF, META_VERSION_1, 0));
    TEST_ASSERT_FALSE(type_gmail(out, 0xFF, META_VERSION_1, PASSWORD_MAX_SIZE + 1));
    TEST_ASSERT_EQUAL(0, hash_calls);
    TEST_ASSERT_EQUAL(0, derive_calls);
}

static void test_type_password_nickname_hash_failure(void) {
    failing_hash = 1;

    TEST_ASSERT_FALSE(type_gmail(NULL, 0xFF, META_VERSION_1, 20));
    TEST_ASSERT_EQUAL(0, derive_calls);
    TEST_ASSERT_EQUAL(0, report_count);
}

static void test_type_password_derivation_failure(void) {
    failing_derive = true;

    TEST_ASSERT_FALSE(type_gmail(NULL, 0xFF, META_VERSION_1, 20));
    TEST_ASSERT_EQUAL(1, derive_calls);
    TEST_ASSERT_EQUAL(1, hash_calls);
    TEST_ASSERT_EQUAL(0, report_count);
}

static void test_type_password_seed_hash_failure(void) {
    failing_hash = 2;

    TEST_ASSERT_FALSE(type_gmail(NULL, 0xFF, META_VERSION_1, 20));
    TEST_ASSERT_EQUAL(2, hash_calls);
    TEST_ASSERT_EQUAL(0, report_count);
}

/* ----------------------------------------------------------------------- */
/* Typed password                                                          */
/* ----------------------------------------------------------------------- */

static void test_type_password_reports(void) {
    TEST_ASSERT_TRUE(type_gmail(NULL, 0x07, META_VERSION_0, 20));
    check_typed("xNX8IQO4vP0ucO41J6JW", false, false);
}

static void test_type_password_restores_caps_lock(void) {
    G_led_status = 2;

    TEST_ASSERT_TRUE(type_gmail(NULL, 0x07, META_VERSION_0, 20));
    check_typed("xNX8IQO4vP0ucO41J6JW", true, false);
}

static void test_type_password_presses_enter(void) {
    N_storage_rw.press_enter_after_typing = true;

    TEST_ASSERT_TRUE(type_gmail(NULL, 0x07, META_VERSION_0, 20));
    check_typed("xNX8IQO4vP0ucO41J6JW", false, true);
}

// The password holds '^', a dead key on these layouts
static void test_type_password_dead_keys(void) {
    N_storage_rw.keyboard_layout = HID_MAPPING_QWERTY_INTL;
    TEST_ASSERT_TRUE(type_gmail(NULL, 0xFF, META_VERSION_1, 20));
    check_typed("*m8ZlP1|}O vzvJ^QNT4", false, false);

    report_count = 0;
    N_storage_rw.keyboard_layout = HID_MAPPING_QWERTZ;
    TEST_ASSERT_TRUE(type_gmail(NULL, 0xFF, META_VERSION_1, 20));
    check_typed("*m8ZlP1|}O vzvJ^QNT4", false, false);
}

// A layout never set types as QWERTY
static void test_type_password_default_layout(void) {
    static uint8_t qwerty[MAX_REPORTS][REPORT_SIZE];

    TEST_ASSERT_TRUE(type_gmail(NULL, 0xFF, META_VERSION_1, 20));
    const size_t count = report_count;
    memcpy(qwerty, reports, sizeof(reports));

    report_count = 0;
    N_storage_rw.keyboard_layout = HID_MAPPING_NONE;
    TEST_ASSERT_TRUE(type_gmail(NULL, 0xFF, META_VERSION_1, 20));
    TEST_ASSERT_EQUAL(count, report_count);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(qwerty, reports, count * REPORT_SIZE);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_show_password);
    RUN_TEST(test_show_password_wipes_a_failed_generation);
    RUN_TEST(test_type_password_rejects_the_size);
    RUN_TEST(test_type_password_nickname_hash_failure);
    RUN_TEST(test_type_password_derivation_failure);
    RUN_TEST(test_type_password_seed_hash_failure);
    RUN_TEST(test_type_password_reports);
    RUN_TEST(test_type_password_restores_caps_lock);
    RUN_TEST(test_type_password_presses_enter);
    RUN_TEST(test_type_password_dead_keys);
    RUN_TEST(test_type_password_default_layout);
    return UNITY_END();
}
