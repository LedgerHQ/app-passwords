#include <stdint.h>
#include <string.h>

#include "unity.h"

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_set_keyboard_layout_stores_value);
    RUN_TEST(test_set_keyboard_layout_keeps_metadata_count);
    RUN_TEST(test_set_keyboard_layout_rejects_unknown);
    RUN_TEST(test_password_length);
    return UNITY_END();
}
