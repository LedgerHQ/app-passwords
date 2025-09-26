#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include "password_list.h"
#include "types.h"

static int setup(void **state __attribute__((unused))) {
    // resets the whole buffer with initial values
    password_list_reset();
    return 0;
}

static void test_password_list_reset(void **state __attribute__((unused))) {
    for (size_t i = 0; i < MAX_METADATA_COUNT; i++) {
        assert_int_equal(password_list_get_offset(i), 0);
        assert_null(password_list_get_password(i));
    }
}

static void test_password_list_get_offset_ok(void **state __attribute__((unused))) {
    for (size_t i = 0; i < MAX_METADATA_COUNT - 1; i++) {
        assert_int_equal(password_list_get_offset(i), 0);
    }
}

static void test_password_list_get_offset_nok(void **state __attribute__((unused))) {
    const size_t index = MAX_METADATA_COUNT;
    assert_int_equal(password_list_get_offset(index), -1);
}

static void test_password_list_add_password(void **state __attribute__((unused))) {
    const int index = 2, offset = 7;
    const char string[] = "string";

    assert_null(password_list_get_password(index));
    assert_int_equal(password_list_get_offset(index), 0);

    assert_true(password_list_add_password(index, offset, &string[0], sizeof(string)));
    assert_string_equal(password_list_get_password(index), string);
    assert_int_equal(password_list_get_offset(index), offset);
}

static void test_password_list_passwords(void **state __attribute__((unused))) {
    const char *strings[] = {"first", "second", "third", "fourth"};
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        assert_true(password_list_add_password(i, i, strings[i], strlen(strings[i]) + 1));
    }
    const char *const *passwords = password_list_passwords();
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        assert_string_equal(passwords[i], strings[i]);
        assert_string_equal(password_list_get_password(i), strings[i]);
    }
}

static void test_password_list_get_password_nok(void **state __attribute__((unused))) {
    assert_null(password_list_get_password(MAX_METADATA_COUNT + 1));
}

static void test_password_list_passwords_nok(void **state __attribute__((unused))) {
    size_t i;
    for (i = 0; i < MAX_METADATA_COUNT; i++) {
        assert_true(password_list_add_password(i, 0, "whatever", 1));
    }
    // not enough space
    assert_false(password_list_add_password(i, 0, "no more", 1));
}

static void test_password_list_reset_buffer(void **state __attribute__((unused))) {
    const char *strings[] = {"first", "second", "third", "fourth"};
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        assert_true(password_list_add_password(i, i, strings[i], strlen(strings[i]) + 1));
    }
    password_list_reset_buffer();
    const char *const *passwords = password_list_passwords();
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        // all passwords have been removed
        assert_ptr_equal(*passwords[i], NULL);
    }
}

static void test_password_list_set_current(void **state __attribute__((unused))) {
    const char *strings[] = {"first", "second", "third", "fourth"};
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        assert_true(password_list_add_password(i, i * 2, strings[i], strlen(strings[i]) + 1));
    }
    size_t index = 3;
    password_list_set_current(index);
    assert_int_equal(password_list_get_current_offset(), index * 2);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_password_list_reset, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_get_offset_ok, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_get_offset_nok, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_add_password, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_passwords, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_get_password_nok, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_passwords_nok, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_reset_buffer, setup, NULL),
        cmocka_unit_test_setup_teardown(test_password_list_set_current, setup, NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
