#include <string.h>

#include "unity.h"

#include "globals.h"
#include "metadata.h"
#include "tests.h"
#include "fake_vars.h"
#include "Mockpassword_typing.h"

void setUp(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
    init_storage();
}

void tearDown(void) {
}

static void check_nickname(size_t index, const char *expected) {
    const uint32_t offset = get_metadata(index);
    TEST_ASSERT_NOT_EQUAL(UINT32_MAX, offset);
    TEST_ASSERT_EQUAL(strlen(expected), METADATA_NICKNAME_LEN(offset));
    TEST_ASSERT_EQUAL_MEMORY(expected, METADATA_NICKNAME(offset), strlen(expected));
}

static void test_populate_creates_every_password(void) {
    populate_passwords();

    TEST_ASSERT_EQUAL(45, N_storage.metadata_count);
    check_nickname(0, "GitHub");
    check_nickname(19, "a-nickname-19-chars");
    check_nickname(44, "amazon-aws");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_populate_creates_every_password);
    return UNITY_END();
}
