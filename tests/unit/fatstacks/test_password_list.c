#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include "fatstacks/password_list.h"


static int setup(void **state __attribute__((unused))) {
    // resets the whole buffer with initial values
    password_list_reset();
    return 0;
}

static void test_password_list_init(void **state __attribute__((unused))) {
    for (size_t i=0; i < DISPLAYED_PASSWORD_PER_PAGE; i++) {
        assert_int_equal(password_list_get_offset(i), 0);
        assert_ptr_equal(password_list_get_password(i), NULL);
    }
}

static void test_password_list_get_offset_ok(void **state __attribute__((unused))) {
    for (size_t i=0; i < DISPLAYED_PASSWORD_PER_PAGE - 1; i++) {
        assert_int_equal(password_list_get_offset(i), 0);
    }
}

static void test_password_list_get_offset_nok(void **state __attribute__((unused))) {
    const size_t index = DISPLAYED_PASSWORD_PER_PAGE;
    assert_int_equal(password_list_get_offset(index), -1);
}


int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_password_list_init, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_get_offset_ok, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_get_offset_nok, setup, NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
