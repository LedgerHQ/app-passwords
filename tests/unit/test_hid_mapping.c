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
    check_map(map, ALT_KEY, 0x27);
    map_char(HID_MAPPING_AZERTY, '`', map);
    check_map(map, ALT_KEY, 0x24);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_map_char_qwerty_regular, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_qwerty_shift, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_azerty_regular, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_azerty_shift, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_map_char_azerty_alt, NULL, NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
