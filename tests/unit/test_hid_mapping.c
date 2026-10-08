#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include "hid_mapping.h"

const char *test =
    "a&b~c#d  {e\"f'g(h  -i  _j)k=l+m  [n  |o  \\p^q  @r  ]s  }t$u!v:w/x;y.z,A?B  <C  "
    ">D`EFGHIJKLMNOPQRSTUVWXYZ0123456789";

void check_map(uint8_t *map, uint8_t first, uint8_t third) {
    assert_int_equal(map[0], first);
    assert_int_equal(map[1], 0);
    assert_int_equal(map[2], third);
}

static void test_map_char_qwerty_regular(void **state __attribute__((unused))) {
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

static void test_map_char_qwerty_shift(void **state __attribute__((unused))) {
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

static void test_map_char_azerty_regular(void **state __attribute__((unused))) {
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

static void test_map_char_azerty_shift(void **state __attribute__((unused))) {
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

static void test_map_char_azerty_alt(void **state __attribute__((unused))) {
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

static void test_map_char_qwertz_regular(void **state __attribute__((unused))) {
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

static void test_map_char_qwertz_shift(void **state __attribute__((unused))) {
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

static void test_map_char_qwertz_alt(void **state __attribute__((unused))) {
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

static void test_is_dead_key(void **state __attribute__((unused))) {
    assert_true(is_dead_key(HID_MAPPING_QWERTY_INTL, '"'));
    assert_true(is_dead_key(HID_MAPPING_QWERTY_INTL, '\''));
    assert_true(is_dead_key(HID_MAPPING_QWERTY_INTL, '`'));
    assert_true(is_dead_key(HID_MAPPING_QWERTY_INTL, '~'));
    assert_true(is_dead_key(HID_MAPPING_QWERTY_INTL, '^'));
    assert_false(is_dead_key(HID_MAPPING_QWERTY_INTL, 'a'));

    assert_true(is_dead_key(HID_MAPPING_QWERTZ, '^'));
    assert_true(is_dead_key(HID_MAPPING_QWERTZ, '`'));
    assert_false(is_dead_key(HID_MAPPING_QWERTZ, '~'));
    assert_false(is_dead_key(HID_MAPPING_QWERTZ, '"'));

    assert_false(is_dead_key(HID_MAPPING_QWERTY, '^'));
    assert_false(is_dead_key(HID_MAPPING_AZERTY, '^'));
}

static void test_map_char_reports_unmappable(void **state __attribute__((unused))) {
    // map_char() used to raise an exception for keys outside the mapped range, which unwound
    // past the caller's plaintext buffer. It now reports the failure instead.
    uint8_t map[3] = {0xAA, 0xAA, 0xAA};

    assert_false(map_char(HID_MAPPING_QWERTY, 0x00, map));
    assert_false(map_char(HID_MAPPING_QWERTY, 0x1F, map));
    // 0x7f (DEL) maps to index 95, one past the 95 printable entries the table holds. The
    // bounds check used to accept it and read one entry past the key-code table.
    assert_false(map_char(HID_MAPPING_QWERTY, 0x7F, map));
    // `out` is left untouched on failure.
    assert_int_equal(map[0], 0xAA);
    assert_int_equal(map[1], 0xAA);
    assert_int_equal(map[2], 0xAA);

    // The lowest and highest mapped characters still succeed.
    assert_true(map_char(HID_MAPPING_QWERTY, 0x20, map));
    assert_true(map_char(HID_MAPPING_QWERTY, 0x7E, map));
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_map_char_reports_unmappable, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_qwerty_regular, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_qwerty_shift, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_azerty_regular, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_azerty_shift, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_azerty_alt, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_qwertz_regular, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_qwertz_shift, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_qwertz_alt, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_is_dead_key, NULL, NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
