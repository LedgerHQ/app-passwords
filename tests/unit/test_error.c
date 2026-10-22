#include "unity.h"

#include "error.h"

void setUp(void) {
}

void tearDown(void) {
}

static void test_get_error_ok(void) {
    const message_pair_t msg = get_error(OK);

    TEST_ASSERT_NULL(msg.first);
    TEST_ASSERT_NULL(msg.second);
}

static void test_get_error_messages(void) {
    static const struct {
        error_type_t error;
        const char *first;
        const char *second;
    } expected[] = {
        {ERR_NO_MORE_SPACE_AVAILABLE, "Write Error", "Database is full"},
        {ERR_CORRUPTED_METADATA,
         "Write Error",
         "Database should be repaired, please contact Ledger Support"},
        {ERR_NO_METADATA, "Erase Error", "Database already empty"},
        {ERR_METADATA_ENTRY_TOO_BIG, "Write Error", "Entry is too big"},
        {ERR_WEAK_CHARSETS, "Write Error", "Separators alone are too weak"},
        {ERR_PIN_NEEDS_NUMBERS, "Write Error", "PIN lengths need numbers only"},
    };

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        const message_pair_t msg = get_error(expected[i].error);
        TEST_ASSERT_EQUAL_STRING(expected[i].first, msg.first);
        TEST_ASSERT_EQUAL_STRING(expected[i].second, msg.second);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_error_ok);
    RUN_TEST(test_get_error_messages);
    return UNITY_END();
}
