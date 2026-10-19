#include <string.h>

#include "unity.h"

#include "hid_mapping.h"

void setUp(void) {
}

void tearDown(void) {
}

const char *test =
    "a&b~c#d  {e\"f'g(h  -i  _j)k=l+m  [n  |o  \\p^q  @r  ]s  }t$u!v:w/x;y.z,A?B  <C  "
    ">D`EFGHIJKLMNOPQRSTUVWXYZ0123456789";

void check_map(uint8_t *map, uint8_t first, uint8_t third) {
    TEST_ASSERT_EQUAL_HEX8(first, map[0]);
    TEST_ASSERT_EQUAL_HEX8(0, map[1]);
    TEST_ASSERT_EQUAL_HEX8(third, map[2]);
}

static void test_map_char_qwerty_regular(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_QWERTY, 'a', map);
    check_map(map, 0, 0x4);
    map_char(HID_MAPPING_QWERTY, 'z', map);
    check_map(map, 0, 0x1d);
    map_char(HID_MAPPING_QWERTY, 'q', map);
    check_map(map, 0, 0x14);
    map_char(HID_MAPPING_QWERTY, 'w', map);
    check_map(map, 0, 0x1a);
    map_char(HID_MAPPING_QWERTY, '1', map);
    check_map(map, 0, 0x1e);
    map_char(HID_MAPPING_QWERTY, '8', map);
    check_map(map, 0, 0x25);
    map_char(HID_MAPPING_QWERTY, '.', map);
    check_map(map, 0, 0x37);
}

static void test_map_char_qwerty_shift(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_QWERTY, 'A', map);
    check_map(map, SHIFT_KEY, 0x4);
    map_char(HID_MAPPING_QWERTY, 'Z', map);
    check_map(map, SHIFT_KEY, 0x1d);
    map_char(HID_MAPPING_QWERTY, 'Q', map);
    check_map(map, SHIFT_KEY, 0x14);
    map_char(HID_MAPPING_QWERTY, 'W', map);
    check_map(map, SHIFT_KEY, 0x1a);
    map_char(HID_MAPPING_QWERTY, '!', map);
    check_map(map, SHIFT_KEY, 0x1e);
    map_char(HID_MAPPING_QWERTY, '*', map);
    check_map(map, SHIFT_KEY, 0x25);
    map_char(HID_MAPPING_QWERTY, '>', map);
    check_map(map, SHIFT_KEY, 0x37);
}

static void test_map_char_azerty_regular(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_AZERTY, 'q', map);
    check_map(map, 0, 0x4);
    map_char(HID_MAPPING_AZERTY, 'w', map);
    check_map(map, 0, 0x1d);
    map_char(HID_MAPPING_AZERTY, 'a', map);
    check_map(map, 0, 0x14);
    map_char(HID_MAPPING_AZERTY, 'z', map);
    check_map(map, 0, 0x1a);
    map_char(HID_MAPPING_AZERTY, '&', map);
    check_map(map, 0, 0x1e);
    map_char(HID_MAPPING_AZERTY, '_', map);
    check_map(map, 0, 0x25);
    map_char(HID_MAPPING_AZERTY, ':', map);
    check_map(map, 0, 0x37);
}

static void test_map_char_azerty_shift(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_AZERTY, 'Q', map);
    check_map(map, SHIFT_KEY, 0x4);
    map_char(HID_MAPPING_AZERTY, 'W', map);
    check_map(map, SHIFT_KEY, 0x1d);
    map_char(HID_MAPPING_AZERTY, 'A', map);
    check_map(map, SHIFT_KEY, 0x14);
    map_char(HID_MAPPING_AZERTY, 'Z', map);
    check_map(map, SHIFT_KEY, 0x1a);
    map_char(HID_MAPPING_AZERTY, '1', map);
    check_map(map, SHIFT_KEY, 0x1e);
    map_char(HID_MAPPING_AZERTY, '8', map);
    check_map(map, SHIFT_KEY, 0x25);
    map_char(HID_MAPPING_AZERTY, '/', map);
    check_map(map, SHIFT_KEY, 0x37);
}

static void test_map_char_azerty_alt(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_AZERTY, '@', map);
    check_map(map, RIGHT_ALT_KEY, 0x27);
    map_char(HID_MAPPING_AZERTY, '`', map);
    check_map(map, RIGHT_ALT_KEY, 0x24);
    map_char(HID_MAPPING_AZERTY, '[', map);
    check_map(map, RIGHT_ALT_KEY, 0x22);
    map_char(HID_MAPPING_AZERTY, '\\', map);
    check_map(map, RIGHT_ALT_KEY, 0x25);
    map_char(HID_MAPPING_AZERTY, ']', map);
    check_map(map, RIGHT_ALT_KEY, 0x2d);
    map_char(HID_MAPPING_AZERTY, '{', map);
    check_map(map, RIGHT_ALT_KEY, 0x21);
    map_char(HID_MAPPING_AZERTY, '|', map);
    check_map(map, RIGHT_ALT_KEY, 0x23);
    map_char(HID_MAPPING_AZERTY, '}', map);
    check_map(map, RIGHT_ALT_KEY, 0x2e);
    map_char(HID_MAPPING_AZERTY, '~', map);
    check_map(map, RIGHT_ALT_KEY, 0x1f);
}

static void test_map_char_qwertz_regular(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_QWERTZ, 'y', map);
    check_map(map, 0, 0x1d);
    map_char(HID_MAPPING_QWERTZ, 'z', map);
    check_map(map, 0, 0x1c);
    map_char(HID_MAPPING_QWERTZ, 'a', map);
    check_map(map, 0, 0x4);
    map_char(HID_MAPPING_QWERTZ, '-', map);
    check_map(map, 0, 0x38);
    map_char(HID_MAPPING_QWERTZ, '+', map);
    check_map(map, 0, 0x30);
    map_char(HID_MAPPING_QWERTZ, '#', map);
    check_map(map, 0, 0x32);
    map_char(HID_MAPPING_QWERTZ, '<', map);
    check_map(map, 0, 0x64);
    map_char(HID_MAPPING_QWERTZ, '^', map);
    check_map(map, 0, 0x35);
}

static void test_map_char_qwertz_shift(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_QWERTZ, 'Y', map);
    check_map(map, SHIFT_KEY, 0x1d);
    map_char(HID_MAPPING_QWERTZ, 'Z', map);
    check_map(map, SHIFT_KEY, 0x1c);
    map_char(HID_MAPPING_QWERTZ, '"', map);
    check_map(map, SHIFT_KEY, 0x1f);
    map_char(HID_MAPPING_QWERTZ, '/', map);
    check_map(map, SHIFT_KEY, 0x24);
    map_char(HID_MAPPING_QWERTZ, '=', map);
    check_map(map, SHIFT_KEY, 0x27);
    map_char(HID_MAPPING_QWERTZ, '?', map);
    check_map(map, SHIFT_KEY, 0x2d);
    map_char(HID_MAPPING_QWERTZ, '_', map);
    check_map(map, SHIFT_KEY, 0x38);
    map_char(HID_MAPPING_QWERTZ, '\'', map);
    check_map(map, SHIFT_KEY, 0x32);
    map_char(HID_MAPPING_QWERTZ, '`', map);
    check_map(map, SHIFT_KEY, 0x2e);
    map_char(HID_MAPPING_QWERTZ, '>', map);
    check_map(map, SHIFT_KEY, 0x64);
}

static void test_map_char_qwertz_alt(void) {
    uint8_t map[3] = {0, 0, 0};
    map_char(HID_MAPPING_QWERTZ, '@', map);
    check_map(map, RIGHT_ALT_KEY, 0x14);
    map_char(HID_MAPPING_QWERTZ, '{', map);
    check_map(map, RIGHT_ALT_KEY, 0x24);
    map_char(HID_MAPPING_QWERTZ, '[', map);
    check_map(map, RIGHT_ALT_KEY, 0x25);
    map_char(HID_MAPPING_QWERTZ, ']', map);
    check_map(map, RIGHT_ALT_KEY, 0x26);
    map_char(HID_MAPPING_QWERTZ, '}', map);
    check_map(map, RIGHT_ALT_KEY, 0x27);
    map_char(HID_MAPPING_QWERTZ, '\\', map);
    check_map(map, RIGHT_ALT_KEY, 0x2d);
    map_char(HID_MAPPING_QWERTZ, '~', map);
    check_map(map, RIGHT_ALT_KEY, 0x30);
    map_char(HID_MAPPING_QWERTZ, '|', map);
    check_map(map, RIGHT_ALT_KEY, 0x64);
}

static void test_is_dead_key(void) {
    TEST_ASSERT_TRUE(is_dead_key(HID_MAPPING_QWERTY_INTL, '"'));
    TEST_ASSERT_TRUE(is_dead_key(HID_MAPPING_QWERTY_INTL, '\''));
    TEST_ASSERT_TRUE(is_dead_key(HID_MAPPING_QWERTY_INTL, '`'));
    TEST_ASSERT_TRUE(is_dead_key(HID_MAPPING_QWERTY_INTL, '~'));
    TEST_ASSERT_TRUE(is_dead_key(HID_MAPPING_QWERTY_INTL, '^'));
    TEST_ASSERT_FALSE(is_dead_key(HID_MAPPING_QWERTY_INTL, 'a'));

    TEST_ASSERT_TRUE(is_dead_key(HID_MAPPING_QWERTZ, '^'));
    TEST_ASSERT_TRUE(is_dead_key(HID_MAPPING_QWERTZ, '`'));
    TEST_ASSERT_FALSE(is_dead_key(HID_MAPPING_QWERTZ, '~'));
    TEST_ASSERT_FALSE(is_dead_key(HID_MAPPING_QWERTZ, '"'));

    TEST_ASSERT_FALSE(is_dead_key(HID_MAPPING_QWERTY, '^'));
    TEST_ASSERT_FALSE(is_dead_key(HID_MAPPING_AZERTY, '^'));
}

static void test_map_char_reports_unmappable(void) {
    // map_char() used to raise an exception for keys outside the mapped range, which unwound
    // past the caller's plaintext buffer. It now reports the failure instead.
    uint8_t map[3] = {0xAA, 0xAA, 0xAA};

    TEST_ASSERT_FALSE(map_char(HID_MAPPING_QWERTY, 0x00, map));
    TEST_ASSERT_FALSE(map_char(HID_MAPPING_QWERTY, 0x1F, map));
    // 0x7f (DEL) maps to index 95, one past the 95 printable entries the table holds. The
    // bounds check used to accept it and read one entry past the key-code table.
    TEST_ASSERT_FALSE(map_char(HID_MAPPING_QWERTY, 0x7F, map));
    // `out` is left untouched on failure.
    TEST_ASSERT_EQUAL_HEX8(0xAA, map[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, map[1]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, map[2]);

    // The lowest and highest mapped characters still succeed.
    TEST_ASSERT_TRUE(map_char(HID_MAPPING_QWERTY, 0x20, map));
    TEST_ASSERT_TRUE(map_char(HID_MAPPING_QWERTY, 0x7E, map));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_map_char_reports_unmappable);
    RUN_TEST(test_map_char_qwerty_regular);
    RUN_TEST(test_map_char_qwerty_shift);
    RUN_TEST(test_map_char_azerty_regular);
    RUN_TEST(test_map_char_azerty_shift);
    RUN_TEST(test_map_char_azerty_alt);
    RUN_TEST(test_map_char_qwertz_regular);
    RUN_TEST(test_map_char_qwertz_shift);
    RUN_TEST(test_map_char_qwertz_alt);
    RUN_TEST(test_is_dead_key);
    return UNITY_END();
}
