#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "globals.h"
#include "options.h"
#include "password.h"
#include "types.h"
#include "fake_vars.h"

void setUp(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
}

void tearDown(void) {
}

// --- tests ------------------------------------------------------------------

static void test_set_keyboard_layout_stores_value(void) {
    // First write reports that no layout was set yet, later ones do not.
    TEST_ASSERT_TRUE(set_keyboard_layout(HID_MAPPING_AZERTY));
    TEST_ASSERT_EQUAL(HID_MAPPING_AZERTY, N_storage_rw.keyboard_layout);

    TEST_ASSERT_FALSE(set_keyboard_layout(HID_MAPPING_QWERTY_INTL));
    TEST_ASSERT_EQUAL(HID_MAPPING_QWERTY_INTL, N_storage_rw.keyboard_layout);

    TEST_ASSERT_FALSE(set_keyboard_layout(HID_MAPPING_QWERTZ));
    TEST_ASSERT_EQUAL(HID_MAPPING_QWERTZ, N_storage_rw.keyboard_layout);
}

static void test_set_keyboard_layout_keeps_metadata_count(void) {
    // Regression: the setter wrote sizeof(hid_mapping_t) == 4 bytes into the one-byte
    // keyboard_layout field. In internalStorage_t that field is followed by padding and
    // then metadata_count, so a plain layout change in the settings corrupted the
    // persistent password count -- passwords appeared to vanish from the list.
    N_storage_rw.metadata_count = 0x11223344;

    TEST_ASSERT_TRUE(set_keyboard_layout(HID_MAPPING_AZERTY));

    TEST_ASSERT_EQUAL(HID_MAPPING_AZERTY, N_storage_rw.keyboard_layout);
    TEST_ASSERT_EQUAL(0x11223344, N_storage_rw.metadata_count);
}

static void test_set_keyboard_layout_rejects_unknown(void) {
    // map_char() has no mapping for values outside the enum, so they must not reach NVM.
    TEST_ASSERT_FALSE(set_keyboard_layout((hid_mapping_t) 0xEE));
    TEST_ASSERT_EQUAL(HID_MAPPING_NONE, N_storage_rw.keyboard_layout);
}

static void test_password_length(void) {
    // An unset or invalid stored value reads as the default.
    TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, get_password_length());
    N_storage_rw.password_length = 13;
    TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, get_password_length());

    TEST_ASSERT_TRUE(set_password_length(4));
    TEST_ASSERT_EQUAL(4, get_password_length());
    TEST_ASSERT_TRUE(set_password_length(64));
    TEST_ASSERT_EQUAL(64, get_password_length());
    TEST_ASSERT_FALSE(set_password_length(65));
    TEST_ASSERT_EQUAL(64, get_password_length());
}

static void test_charset_options(void) {
    // Uppercase, lowercase and numbers by default
    init_charset_options();
    TEST_ASSERT_EQUAL_HEX8(0x07, get_charset_options());
    TEST_ASSERT_TRUE(has_charset_option(UPPERCASE_BITFLAG));
    TEST_ASSERT_TRUE(has_charset_option(NUMBERS_BITFLAG));
    TEST_ASSERT_FALSE(has_charset_option(BARS_BITFLAG));

    // An option is toggled, the others are kept
    set_charset_option(NUMBERS_BITFLAG);
    TEST_ASSERT_EQUAL_HEX8(0x03, get_charset_options());
    set_charset_option(BARS_BITFLAG);
    TEST_ASSERT_EQUAL_HEX8(0x3B, get_charset_options());
    TEST_ASSERT_EQUAL_HEX8(0x3B, N_storage_rw.charset_options);
    set_charset_option(NUMBERS_BITFLAG);
    set_charset_option(BARS_BITFLAG);
    TEST_ASSERT_EQUAL_HEX8(0x07, get_charset_options());
}

static void test_change_enter_options(void) {
    change_enter_options();
    TEST_ASSERT_TRUE(N_storage_rw.press_enter_after_typing);
    change_enter_options();
    TEST_ASSERT_FALSE(N_storage_rw.press_enter_after_typing);
}

static void test_init_storage_on_a_blank_storage(void) {
    // Whatever the flash held before the first start
    memset(&N_storage_rw, 0xAA, sizeof(N_storage_rw));

    TEST_ASSERT_TRUE(init_storage());

    TEST_ASSERT_EQUAL_HEX32(STORAGE_MAGIC, N_storage_rw.magic);
    TEST_ASSERT_FALSE(N_storage_rw.press_enter_after_typing);
    TEST_ASSERT_EQUAL(HID_MAPPING_NONE, N_storage_rw.keyboard_layout);
    // init_storage() writes the count from a uint32_t: only the 4 bytes of the device's size_t
    TEST_ASSERT_EQUAL_HEX32(0, (uint32_t) N_storage_rw.metadata_count);
    TEST_ASSERT_EQUAL(0, N_storage_rw.restore_in_progress);
    TEST_ASSERT_EQUAL_HEX8(0x07, N_storage_rw.charset_options);
    TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, N_storage_rw.password_length);
    for (size_t i = 0; i < sizeof(N_storage_rw.metadatas); i++) {
        TEST_ASSERT_EQUAL_HEX8(0, N_storage_rw.metadatas[i]);
    }
}

static void test_init_storage_keeps_an_initialized_storage(void) {
    N_storage_rw.magic = STORAGE_MAGIC;
    N_storage_rw.press_enter_after_typing = true;
    N_storage_rw.keyboard_layout = HID_MAPPING_AZERTY;
    N_storage_rw.metadata_count = 1;
    N_storage_rw.metadatas[0] = 0x02;
    N_storage_rw.charset_options = 0xFF;
    N_storage_rw.password_length = 64;
    const internalStorage_t before = N_storage_rw;

    TEST_ASSERT_FALSE(init_storage());
    TEST_ASSERT_EQUAL_MEMORY(&before, &N_storage_rw, sizeof(before));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_set_keyboard_layout_stores_value);
    RUN_TEST(test_set_keyboard_layout_keeps_metadata_count);
    RUN_TEST(test_set_keyboard_layout_rejects_unknown);
    RUN_TEST(test_password_length);
    RUN_TEST(test_charset_options);
    RUN_TEST(test_change_enter_options);
    RUN_TEST(test_init_storage_on_a_blank_storage);
    RUN_TEST(test_init_storage_keeps_an_initialized_storage);
    return UNITY_END();
}
