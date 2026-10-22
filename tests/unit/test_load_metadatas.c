#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "handlers.h"
#include "metadata.h"
#include "types.h"
#include "fake_vars.h"
#include "Mockio.h"
#include "Mockui.h"

// Entries "a" and "allah", then the terminator
static const uint8_t RESTORED_IMAGE[] =
    {0x02, META_NONE, 0x07, 'a', 0x06, META_NONE, 0x07, 'a', 'l', 'l', 'a', 'h'};

static uint16_t last_sw;
static size_t sw_sent;
static size_t approvals_requested;
static size_t idle_calls;

static int fake_io_send_response_buffers(const buffer_t *rdatalist,
                                         size_t count,
                                         uint16_t sw,
                                         int cmock_num_calls) {
    (void) rdatalist;
    (void) cmock_num_calls;
    // The handler only answers status words
    TEST_ASSERT_EQUAL(0, count);
    last_sw = sw;
    sw_sent++;
    return 0;
}

static void fake_ui_idle(int cmock_num_calls) {
    (void) cmock_num_calls;
    idle_calls++;
}

static void fake_ui_request_user_approval(message_pair_t *msg, int cmock_num_calls) {
    (void) msg;
    (void) cmock_num_calls;
    approvals_requested++;
}

static int send_chunk(uint8_t p1, const uint8_t *data, size_t size) {
    const buf_t input = {.bytes = (uint8_t *) data, .size = size};
    return load_metadatas(p1, 0, &input);
}

// Approve the restore, as the user does on the first command
static void approve(void) {
    TEST_ASSERT_EQUAL(0, send_chunk(0, NULL, 0));
    TEST_ASSERT_EQUAL(1, approvals_requested);
    TEST_ASSERT_EQUAL(0, sw_sent);
    app_state.user_approval = true;
}

// A database holding the single entry "before"
static void store_before(void) {
    static const uint8_t before[] = {0x07, META_NONE, 0x07, 'b', 'e', 'f', 'o', 'r', 'e'};
    memcpy(N_storage_rw.metadatas, before, sizeof(before));
    N_storage_rw.metadata_count = 1;
}

static void assert_database_is_before(void) {
    TEST_ASSERT_EQUAL(1, N_storage_rw.metadata_count);
    TEST_ASSERT_EQUAL(0x07, N_storage_rw.metadatas[0]);
    TEST_ASSERT_EQUAL_MEMORY("before", &N_storage_rw.metadatas[3], 6);
}

static void assert_database_is_image(void) {
    TEST_ASSERT_EQUAL(2, N_storage_rw.metadata_count);
    TEST_ASSERT_EQUAL_MEMORY(RESTORED_IMAGE, N_storage_rw.metadatas, sizeof(RESTORED_IMAGE));
    for (size_t i = sizeof(RESTORED_IMAGE); i < MAX_METADATAS; i++) {
        TEST_ASSERT_EQUAL(0, N_storage_rw.metadatas[i]);
    }
}

void setUp(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
    memset(&app_state, 0, sizeof(app_state));
    last_sw = 0;
    sw_sent = 0;
    approvals_requested = 0;
    idle_calls = 0;
    discard_metadata_restore();
    store_before();
    Mockio_Init();
    Mockui_Init();
    io_send_response_buffers_StubWithCallback(fake_io_send_response_buffers);
    ui_idle_StubWithCallback(fake_ui_idle);
    ui_request_user_approval_StubWithCallback(fake_ui_request_user_approval);
}

void tearDown(void) {
    Mockio_Verify();
    Mockui_Verify();
    Mockio_Destroy();
    Mockui_Destroy();
}

/* ----------------------------------------------------------------------- */
/* Approval and parameters                                                 */
/* ----------------------------------------------------------------------- */

static void test_load_asks_approval_first(void) {
    // Data sent before the approval is dropped
    TEST_ASSERT_EQUAL(0, send_chunk(LAST_CHUNK, RESTORED_IMAGE, sizeof(RESTORED_IMAGE)));
    TEST_ASSERT_EQUAL(1, approvals_requested);
    TEST_ASSERT_EQUAL(0, sw_sent);
    TEST_ASSERT_EQUAL(0, app_state.bytes_transferred);
    assert_database_is_before();
}

static void test_load_rejects_wrong_p1_p2(void) {
    const buf_t input = {.bytes = (uint8_t *) RESTORED_IMAGE, .size = sizeof(RESTORED_IMAGE)};

    approve();
    TEST_ASSERT_EQUAL(0, load_metadatas(0x01, 0, &input));
    TEST_ASSERT_EQUAL_HEX16(SWO_INCORRECT_P1_P2, last_sw);
    TEST_ASSERT_EQUAL(0, load_metadatas(0, 0x01, &input));
    TEST_ASSERT_EQUAL_HEX16(SWO_INCORRECT_P1_P2, last_sw);
    TEST_ASSERT_EQUAL(0, app_state.bytes_transferred);
    assert_database_is_before();
}

/* ----------------------------------------------------------------------- */
/* Transfers                                                               */
/* ----------------------------------------------------------------------- */

static void test_load_short_image(void) {
    approve();
    TEST_ASSERT_EQUAL(0, send_chunk(LAST_CHUNK, RESTORED_IMAGE, sizeof(RESTORED_IMAGE)));

    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, last_sw);
    TEST_ASSERT_EQUAL(1, idle_calls);
    TEST_ASSERT_FALSE(app_state.user_approval);
    TEST_ASSERT_EQUAL(0, app_state.bytes_transferred);
    TEST_ASSERT_FALSE(metadata_restore_in_progress());
    assert_database_is_image();
}

// A full image is committed on its last byte, without the last chunk flag
static void test_load_full_image_in_chunks(void) {
    static uint8_t image[MAX_METADATAS];
    memcpy(image, RESTORED_IMAGE, sizeof(RESTORED_IMAGE));

    approve();
    size_t sent = 0;
    while (sent < sizeof(image)) {
        const size_t size = (sizeof(image) - sent < 255) ? sizeof(image) - sent : 255;
        TEST_ASSERT_EQUAL(0, idle_calls);
        TEST_ASSERT_EQUAL(0, send_chunk(0, &image[sent], size));
        TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, last_sw);
        sent += size;
    }

    TEST_ASSERT_EQUAL(1, idle_calls);
    TEST_ASSERT_FALSE(app_state.user_approval);
    assert_database_is_image();
}

static void test_load_one_byte_too_many(void) {
    static uint8_t image[MAX_METADATAS + 1];
    memcpy(image, RESTORED_IMAGE, sizeof(RESTORED_IMAGE));

    approve();
    TEST_ASSERT_EQUAL(0, send_chunk(0, image, MAX_METADATAS - 6));
    TEST_ASSERT_EQUAL(0, send_chunk(0, &image[MAX_METADATAS - 6], 7));

    TEST_ASSERT_EQUAL_HEX16(SWO_WRONG_DATA_LENGTH, last_sw);
    TEST_ASSERT_EQUAL(0, app_state.bytes_transferred);
    TEST_ASSERT_EQUAL(0, idle_calls);
    assert_database_is_before();
}

static void test_load_rejects_an_invalid_image(void) {
    // A record claiming more data than a nickname holds
    const uint8_t invalid[] = {MAX_METANAME + 1, META_NONE, 0x07, 'a'};

    approve();
    TEST_ASSERT_EQUAL(0, send_chunk(LAST_CHUNK, invalid, sizeof(invalid)));

    TEST_ASSERT_EQUAL_HEX16(SW_METADATAS_PARSING_ERROR, last_sw);
    TEST_ASSERT_FALSE(app_state.user_approval);
    TEST_ASSERT_EQUAL(0, app_state.bytes_transferred);
    TEST_ASSERT_FALSE(metadata_restore_in_progress());
    assert_database_is_before();
}

// A failed transfer leaves nothing behind for the next one
static void test_load_after_an_aborted_transfer(void) {
    static uint8_t stale[64];
    memset(stale, 'z', sizeof(stale));

    approve();
    TEST_ASSERT_EQUAL(0, send_chunk(0, stale, sizeof(stale)));
    TEST_ASSERT_EQUAL(0, load_metadatas(0x01, 0, &(const buf_t) {.bytes = stale, .size = 1}));
    TEST_ASSERT_EQUAL_HEX16(SWO_INCORRECT_P1_P2, last_sw);

    TEST_ASSERT_EQUAL(0, send_chunk(LAST_CHUNK, RESTORED_IMAGE, sizeof(RESTORED_IMAGE)));
    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, last_sw);
    assert_database_is_image();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_load_asks_approval_first);
    RUN_TEST(test_load_rejects_wrong_p1_p2);
    RUN_TEST(test_load_short_image);
    RUN_TEST(test_load_full_image_in_chunks);
    RUN_TEST(test_load_one_byte_too_many);
    RUN_TEST(test_load_rejects_an_invalid_image);
    RUN_TEST(test_load_after_an_aborted_transfer);
    return UNITY_END();
}
