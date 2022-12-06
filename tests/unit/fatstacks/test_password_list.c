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
    assert_int_equal(password_list_get_index(), 0);
    for (int i=0; i < DISPLAYED_PASSWORD_PER_PAGE; i++) {
        assert_int_equal(password_list_get_offset(i), 0);
        assert_ptr_equal(password_list_get_password(i), NULL);
    }
    assert_int_equal(password_list_get_current_offset(), 0);
}

static void test_password_list_set_index_ok(void **state __attribute__((unused))) {
    const int new_index = 3;
    assert_int_equal(password_list_get_index(), 0);
    password_list_set_index(new_index);
    assert_int_equal(password_list_get_index(), new_index);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_password_list_init, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_set_index_ok, setup, NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
