#include <string.h>

#include "unity.h"

#include "password_list.h"
#include "types.h"

void setUp(void) {
    // resets the whole buffer with initial values
    password_list_reset();
}

void tearDown(void) {
}

static void test_password_list_reset(void) {
    for (size_t i = 0; i < MAX_METADATA_COUNT; i++) {
        TEST_ASSERT_EQUAL(0, password_list_get_offset(i));
        TEST_ASSERT_NULL(password_list_get_password(i));
    }
}

static void test_password_list_get_offset_ok(void) {
    for (size_t i = 0; i < MAX_METADATA_COUNT - 1; i++) {
        TEST_ASSERT_EQUAL(0, password_list_get_offset(i));
    }
}

static void test_password_list_get_offset_nok(void) {
    const size_t index = MAX_METADATA_COUNT;
    TEST_ASSERT_EQUAL(-1, password_list_get_offset(index));
}

static void test_password_list_add_password(void) {
    const int index = 2, offset = 7;
    const char string[] = "string";

    TEST_ASSERT_NULL(password_list_get_password(index));
    TEST_ASSERT_EQUAL(0, password_list_get_offset(index));

    TEST_ASSERT_TRUE(password_list_add_password(index, offset, &string[0], sizeof(string)));
    TEST_ASSERT_EQUAL_STRING(string, password_list_get_password(index));
    TEST_ASSERT_EQUAL(offset, password_list_get_offset(index));
}

static void test_password_list_passwords(void) {
    const char *strings[] = {"first", "second", "third", "fourth"};
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        TEST_ASSERT_TRUE(password_list_add_password(i, i, strings[i], strlen(strings[i]) + 1));
    }
    const char *const *passwords = password_list_passwords();
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        TEST_ASSERT_EQUAL_STRING(strings[i], passwords[i]);
        TEST_ASSERT_EQUAL_STRING(strings[i], password_list_get_password(i));
    }
}

static void test_password_list_get_password_nok(void) {
    TEST_ASSERT_NULL(password_list_get_password(MAX_METADATA_COUNT + 1));
}

static void test_password_list_passwords_nok(void) {
    size_t i;
    for (i = 0; i < MAX_METADATA_COUNT; i++) {
        TEST_ASSERT_TRUE(password_list_add_password(i, 0, "whatever", 1));
    }
    // not enough space
    TEST_ASSERT_FALSE(password_list_add_password(i, 0, "no more", 1));
}

static void test_password_list_reset_buffer(void) {
    const char *strings[] = {"first", "second", "third", "fourth"};
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        TEST_ASSERT_TRUE(password_list_add_password(i, i, strings[i], strlen(strings[i]) + 1));
    }
    password_list_reset_buffer();
    const char *const *passwords = password_list_passwords();
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        // all passwords have been removed
        TEST_ASSERT_EQUAL_CHAR('\0', passwords[i][0]);
    }
}

static void test_password_list_set_current(void) {
    const char *strings[] = {"first", "second", "third", "fourth"};
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        TEST_ASSERT_TRUE(password_list_add_password(i, i * 2, strings[i], strlen(strings[i]) + 1));
    }
    size_t index = 3;
    password_list_set_current(index);
    TEST_ASSERT_EQUAL(index * 2, password_list_get_current_offset());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_password_list_reset);
    RUN_TEST(test_password_list_get_offset_ok);
    RUN_TEST(test_password_list_get_offset_nok);
    RUN_TEST(test_password_list_add_password);
    RUN_TEST(test_password_list_passwords);
    RUN_TEST(test_password_list_get_password_nok);
    RUN_TEST(test_password_list_passwords_nok);
    RUN_TEST(test_password_list_reset_buffer);
    RUN_TEST(test_password_list_set_current);
    return UNITY_END();
}
