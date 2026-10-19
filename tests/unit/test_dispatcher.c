#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "dispatcher.h"
#include "offsets.h"
#include "types.h"
#include "fake_vars.h"
#include "Mockhandlers.h"
#include "Mockio.h"

static uint16_t last_sw;
static size_t sw_sent;

static int fake_io_send_response_buffers(const buffer_t *rdatalist,
                                         size_t count,
                                         uint16_t sw,
                                         int cmock_num_calls) {
    (void) rdatalist;
    (void) cmock_num_calls;
    TEST_ASSERT_EQUAL(0, count);
    last_sw = sw;
    sw_sent++;
    return 0;
}

// Arguments of the last handler call taking parameters
static struct {
    size_t calls;
    uint8_t p1;
    uint8_t p2;
    buf_t input;
} handled;

static int fake_handler(uint8_t p1, uint8_t p2, const buf_t *input, int cmock_num_calls) {
    (void) cmock_num_calls;
    handled.calls++;
    handled.p1 = p1;
    handled.p2 = p2;
    handled.input = *input;
    return 0x1234;
}

static void load_apdu(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t lc) {
    G_io_apdu_buffer[OFFSET_CLA] = cla;
    G_io_apdu_buffer[OFFSET_INS] = ins;
    G_io_apdu_buffer[OFFSET_P1] = p1;
    G_io_apdu_buffer[OFFSET_P2] = p2;
    G_io_apdu_buffer[OFFSET_LC] = lc;
}

void setUp(void) {
    memset(&app_state, 0, sizeof(app_state));
    memset(G_io_apdu_buffer, 0, sizeof(G_io_apdu_buffer));
    memset(&handled, 0, sizeof(handled));
    last_sw = 0;
    sw_sent = 0;
    Mockhandlers_Init();
    Mockio_Init();
    io_send_response_buffers_StubWithCallback(fake_io_send_response_buffers);
}

void tearDown(void) {
    Mockhandlers_Verify();
    Mockio_Verify();
    Mockhandlers_Destroy();
    Mockio_Destroy();
}

/* ----------------------------------------------------------------------- */
/* Rejected commands                                                       */
/* ----------------------------------------------------------------------- */

static void test_dispatch_rejects_the_class(void) {
    app_state.current_command = LOAD_METADATAS;
    app_state.user_approval = true;
    load_apdu(CLA + 1, DUMP_METADATAS, 0, 0, 0);

    TEST_ASSERT_EQUAL(0, dispatch());
    TEST_ASSERT_EQUAL_HEX16(SWO_INVALID_CLA, last_sw);
    // The command in progress is kept
    TEST_ASSERT_EQUAL(LOAD_METADATAS, app_state.current_command);
    TEST_ASSERT_TRUE(app_state.user_approval);
}

static void test_dispatch_rejects_unknown_instructions(void) {
    static const uint8_t unknown[] = {0x00, 0x01, 0x02, 0x06, 0x99, 0xFF};
    for (size_t i = 0; i < sizeof(unknown); i++) {
        load_apdu(CLA, unknown[i], 0, 0, 0);
        TEST_ASSERT_EQUAL(0, dispatch());
        TEST_ASSERT_EQUAL_HEX16(SWO_INVALID_INS, last_sw);
    }
    TEST_ASSERT_EQUAL(sizeof(unknown), sw_sent);
}

/* ----------------------------------------------------------------------- */
/* Routing                                                                 */
/* ----------------------------------------------------------------------- */

static void test_dispatch_get_app_config(void) {
    get_app_config_StubWithCallback(fake_handler);
    load_apdu(CLA, GET_APP_CONFIG, 0x01, 0x02, 3);

    TEST_ASSERT_EQUAL(0x1234, dispatch());
    TEST_ASSERT_EQUAL(1, handled.calls);
    TEST_ASSERT_EQUAL(0x01, handled.p1);
    TEST_ASSERT_EQUAL(0x02, handled.p2);
    TEST_ASSERT_EQUAL_PTR(&G_io_apdu_buffer[OFFSET_CDATA], handled.input.bytes);
    TEST_ASSERT_EQUAL(3, handled.input.size);
}

static void test_dispatch_dump_metadatas(void) {
    dump_metadatas_ExpectAndReturn(0x1234);
    load_apdu(CLA, DUMP_METADATAS, 0, 0, 0);

    TEST_ASSERT_EQUAL(0x1234, dispatch());
    TEST_ASSERT_EQUAL(0, sw_sent);
}

static void test_dispatch_load_metadatas(void) {
    load_metadatas_StubWithCallback(fake_handler);
    load_apdu(CLA, LOAD_METADATAS, 0xFF, 0x00, 0xFF);

    TEST_ASSERT_EQUAL(0x1234, dispatch());
    TEST_ASSERT_EQUAL(1, handled.calls);
    TEST_ASSERT_EQUAL(0xFF, handled.p1);
    TEST_ASSERT_EQUAL(0x00, handled.p2);
    TEST_ASSERT_EQUAL_PTR(&G_io_apdu_buffer[OFFSET_CDATA], handled.input.bytes);
    TEST_ASSERT_EQUAL(0xFF, handled.input.size);
}

/* ----------------------------------------------------------------------- */
/* Command state                                                           */
/* ----------------------------------------------------------------------- */

// The approval is given for one command, and kept while it goes on
static void test_dispatch_approval_follows_the_command(void) {
    app_state.current_command = DUMP_METADATAS;
    app_state.user_approval = true;

    dump_metadatas_ExpectAndReturn(0);
    load_apdu(CLA, DUMP_METADATAS, 0, 0, 0);
    dispatch();
    TEST_ASSERT_TRUE(app_state.user_approval);

    get_app_config_IgnoreAndReturn(0);
    load_apdu(CLA, GET_APP_CONFIG, 0, 0, 0);
    dispatch();
    TEST_ASSERT_EQUAL(GET_APP_CONFIG, app_state.current_command);
    TEST_ASSERT_FALSE(app_state.user_approval);
}

static void test_dispatch_discards_an_interrupted_restore(void) {
    app_state.current_command = LOAD_METADATAS;
    app_state.user_approval = true;

    discard_metadata_restore_Expect();
    dump_metadatas_ExpectAndReturn(0);
    load_apdu(CLA, DUMP_METADATAS, 0, 0, 0);
    dispatch();
    TEST_ASSERT_EQUAL(DUMP_METADATAS, app_state.current_command);
    TEST_ASSERT_FALSE(app_state.user_approval);
}

// Only a restore holds a partial image
static void test_dispatch_discards_only_a_restore(void) {
    app_state.current_command = DUMP_METADATAS;

    load_metadatas_IgnoreAndReturn(0);
    load_apdu(CLA, LOAD_METADATAS, 0, 0, 0);
    dispatch();
    TEST_ASSERT_EQUAL(LOAD_METADATAS, app_state.current_command);

    // A rejected instruction still interrupts the restore
    discard_metadata_restore_Expect();
    load_apdu(CLA, 0x42, 0, 0, 0);
    dispatch();
    TEST_ASSERT_EQUAL_HEX16(SWO_INVALID_INS, last_sw);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dispatch_rejects_the_class);
    RUN_TEST(test_dispatch_rejects_unknown_instructions);
    RUN_TEST(test_dispatch_get_app_config);
    RUN_TEST(test_dispatch_dump_metadatas);
    RUN_TEST(test_dispatch_load_metadatas);
    RUN_TEST(test_dispatch_approval_follows_the_command);
    RUN_TEST(test_dispatch_discards_an_interrupted_restore);
    RUN_TEST(test_dispatch_discards_only_a_restore);
    return UNITY_END();
}
