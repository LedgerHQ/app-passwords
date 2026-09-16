#include <stdarg.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "options.h"
#include "types.h"

// See test_password.c for why N_storage_real is defined without `const` here.
internalStorage_t N_storage_real;

void nvm_write(void *dst, void *src, unsigned int len) {
    if (src == NULL) {
        memset(dst, 0, len);
    } else {
        memcpy(dst, src, len);
    }
}

static int setup(void **state __attribute__((unused))) {
    memset(&N_storage_real, 0, sizeof(N_storage_real));
    return 0;
}

// --- tests ------------------------------------------------------------------

static void test_set_keyboard_layout_stores_value(void **state __attribute__((unused))) {
    // First write reports that no layout was set yet, later ones do not.
    assert_true(set_keyboard_layout(HID_MAPPING_AZERTY));
    assert_int_equal(N_storage_real.keyboard_layout, HID_MAPPING_AZERTY);

    assert_false(set_keyboard_layout(HID_MAPPING_QWERTY_INTL));
    assert_int_equal(N_storage_real.keyboard_layout, HID_MAPPING_QWERTY_INTL);
}

static void test_set_keyboard_layout_keeps_metadata_count(void **state __attribute__((unused))) {
    // Regression: the setter wrote sizeof(hid_mapping_t) == 4 bytes into the one-byte
    // keyboard_layout field. In internalStorage_t that field is followed by padding and
    // then metadata_count, so a plain layout change in the settings corrupted the
    // persistent password count -- passwords appeared to vanish from the list.
    N_storage_real.metadata_count = 0x11223344;

    assert_true(set_keyboard_layout(HID_MAPPING_AZERTY));

    assert_int_equal(N_storage_real.keyboard_layout, HID_MAPPING_AZERTY);
    assert_int_equal(N_storage_real.metadata_count, 0x11223344);
}

static void test_set_keyboard_layout_rejects_unknown(void **state __attribute__((unused))) {
    // map_char() has no mapping for values outside the enum, so they must not reach NVM.
    assert_false(set_keyboard_layout((hid_mapping_t) 0xEE));
    assert_int_equal(N_storage_real.keyboard_layout, HID_MAPPING_NONE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_set_keyboard_layout_stores_value, setup, NULL),
        cmocka_unit_test_setup_teardown(test_set_keyboard_layout_keeps_metadata_count, setup, NULL),
        cmocka_unit_test_setup_teardown(test_set_keyboard_layout_rejects_unknown, setup, NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
