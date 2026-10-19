#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "handlers.h"
#include "metadata.h"
#include "types.h"
#include "fake_vars.h"
#include "Mockio.h"
#include "Mockui.h"

// --- captured side effects --------------------------------------------------

// Everything dump_metadatas() handed to the IO layer, concatenated in call order. The device
// sends the store as a sequence of chunks, and the leak this file guards against is about what
// the *whole* stream contains, so the assertions run over the reassembled payload.
static uint8_t sent_payload[sizeof(N_storage_real.metadatas) * 2];
static size_t sent_payload_len;
static size_t responses_sent;
static uint16_t last_sw;
static size_t approvals_requested;
static size_t idle_calls;

static int fake_io_send_response_buffers(const buffer_t *rdatalist,
                                         size_t count,
                                         uint16_t sw,
                                         int cmock_num_calls) {
    (void) cmock_num_calls;
    last_sw = sw;
    if (count == 0) {
        return 0;
    }
    TEST_ASSERT_EQUAL(1, count);
    const size_t size = rdatalist[0].size;
    TEST_ASSERT_TRUE(size >= TRANSFER_PAYLOAD_OFFSET);
    const size_t payload_size = size - TRANSFER_PAYLOAD_OFFSET;
    TEST_ASSERT_TRUE(sent_payload_len + payload_size <= sizeof(sent_payload));
    memcpy(&sent_payload[sent_payload_len],
           rdatalist[0].ptr + TRANSFER_PAYLOAD_OFFSET,
           payload_size);
    sent_payload_len += payload_size;
    responses_sent++;
    return (int) size;
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

// --- helpers ----------------------------------------------------------------

// Append one live metadata entry holding `nickname` to N_storage_real.
static void add_password(const char *nickname) {
    const size_t name_len = strlen(nickname);
    const uint8_t datasize = 1 + name_len;  // charset byte + nickname bytes
    uint32_t offset = 0;
    while (N_storage_rw.metadatas[offset] != 0) {
        offset += N_storage_rw.metadatas[offset] + 2;
    }
    N_storage_rw.metadatas[offset] = datasize;
    N_storage_rw.metadatas[offset + 1] = META_NONE;
    N_storage_rw.metadatas[offset + 2] = 0;  // charset byte
    memcpy(&N_storage_rw.metadatas[offset + 3], nickname, name_len);
    // terminate the list so the parsing loops stop here
    N_storage_rw.metadatas[offset + 3 + name_len] = 0;
    N_storage_rw.metadata_count++;
}

static uint32_t raw_entry_offset(size_t nth) {
    uint32_t offset = 0;
    while (nth-- > 0) {
        TEST_ASSERT_NOT_EQUAL(0, N_storage_rw.metadatas[offset]);
        offset += N_storage_rw.metadatas[offset] + 2;
    }
    return offset;
}

// Mark the record at `offset` erased the way an app version that did not wipe the data block
// did: flip the kind byte, leave the nickname in flash. No current code path produces this --
// erase_metadata() wipes the block -- but a database carried across an upgrade contains such
// records, which is the case this file is about.
static void erase_metadata_legacy_style(uint32_t offset) {
    N_storage_rw.metadatas[offset + 1] = META_ERASED;
    TEST_ASSERT_TRUE(N_storage_rw.metadata_count > 0);
    N_storage_rw.metadata_count--;
}

// True if `needle` appears anywhere in `buffer`.
static bool contains(const uint8_t *buffer, size_t size, const char *needle) {
    const size_t needle_len = strlen(needle);
    if (needle_len > size) {
        return false;
    }
    for (size_t i = 0; i + needle_len <= size; i++) {
        if (memcmp(&buffer[i], needle, needle_len) == 0) {
            return true;
        }
    }
    return false;
}

// Drive a whole backup: the approval request, then every chunk until the handler reports the
// last one. Returns the number of chunks the transfer took.
static size_t run_full_dump(void) {
    TEST_ASSERT_EQUAL(0, dump_metadatas());
    // The first call only asks for approval; nothing may be exported before the user agrees.
    TEST_ASSERT_EQUAL(1, approvals_requested);
    TEST_ASSERT_EQUAL(0, responses_sent);

    app_state.user_approval = true;
    size_t chunks = 0;
    while (app_state.user_approval) {
        TEST_ASSERT_EQUAL(0, dump_metadatas());
        chunks++;
        // A stuck handler must not spin forever on a failed assertion elsewhere.
        TEST_ASSERT_TRUE(chunks <= sizeof(N_storage_real.metadatas));
    }
    return chunks;
}

void setUp(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
    memset(&app_state, 0, sizeof(app_state));
    memset(G_io_apdu_buffer, 0, sizeof(G_io_apdu_buffer));
    memset(sent_payload, 0, sizeof(sent_payload));
    sent_payload_len = 0;
    responses_sent = 0;
    last_sw = 0;
    approvals_requested = 0;
    idle_calls = 0;
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

// --- tests ------------------------------------------------------------------

static void test_dump_does_not_export_a_legacy_erased_nickname(void) {
    // The exported region is bounded at the terminator, but a legacy erased record sits
    // *before* it, so its nickname was inside the bound and went out with the backup. The
    // handler compacts the store before the first chunk, which drops the record.
    add_password("alpha");
    add_password("secretsite");
    add_password("gamma");
    erase_metadata_legacy_style(raw_entry_offset(1));

    run_full_dump();

    TEST_ASSERT_EQUAL(SWO_SUCCESS, last_sw);
    TEST_ASSERT_FALSE(contains(sent_payload, sent_payload_len, "secretsite"));
    // The surviving names are still exported, so the backup is not simply empty.
    TEST_ASSERT_TRUE(contains(sent_payload, sent_payload_len, "alpha"));
    TEST_ASSERT_TRUE(contains(sent_payload, sent_payload_len, "gamma"));
}

static void test_dump_exports_the_whole_region(void) {
    add_password("alpha");

    const size_t chunks = run_full_dump();

    // The host reads a fixed-size store, so the stream must cover it exactly whatever the
    // chunk size works out to.
    TEST_ASSERT_EQUAL(sizeof(N_storage_real.metadatas), sent_payload_len);
    TEST_ASSERT_EQUAL(responses_sent, chunks);
    TEST_ASSERT_EQUAL(1, idle_calls);  // only the last chunk returns to the idle screen
}

static void test_dump_exports_live_records_verbatim(void) {
    add_password("alpha");
    add_password("beta");

    run_full_dump();

    /* Records are [datalen][kind][charset][nickname], then the two-byte terminator. Written as
     * adjacent literals so a "\x00" escape cannot swallow the letter that follows it: in one
     * literal, "\x00alpha" would parse as the single hex character \x0a. */
    static const char expected[] =
        "\x06\x00\x00"
        "alpha"
        "\x05\x00\x00"
        "beta"
        "\x00\x00";
    const size_t expected_len = sizeof(expected) - 1;  // drop the implicit terminator

    TEST_ASSERT_EQUAL_MEMORY(expected, sent_payload, expected_len);
    // Everything past the terminator is zeroed rather than raw flash slack.
    for (size_t i = expected_len; i < sent_payload_len; i++) {
        TEST_ASSERT_EQUAL(0, sent_payload[i]);
    }
}

static void test_dump_zeroes_the_slack_past_the_terminator(void) {
    add_password("alpha");
    // Stale bytes of a name that is no longer part of the database, sitting past its end.
    const char stale[] = "oldsecret";
    memcpy(&N_storage_rw.metadatas[200], stale, sizeof(stale));

    run_full_dump();

    TEST_ASSERT_FALSE(contains(sent_payload, sent_payload_len, stale));
}

static void test_dump_refuses_a_store_that_does_not_parse(void) {
    // A record claiming more data than MAX_METANAME allows: the store has no usable
    // terminator, so no length bound can be trusted and nothing may be exported.
    N_storage_rw.metadatas[0] = MAX_METANAME + 1;
    N_storage_rw.metadatas[1] = META_NONE;
    N_storage_rw.metadata_count = 1;

    TEST_ASSERT_EQUAL(0, dump_metadatas());
    app_state.user_approval = true;
    TEST_ASSERT_EQUAL(0, dump_metadatas());

    TEST_ASSERT_EQUAL(SW_METADATAS_PARSING_ERROR, last_sw);
    TEST_ASSERT_EQUAL(0, responses_sent);
    // The transfer is abandoned, not left half-open for the next command.
    TEST_ASSERT_FALSE(app_state.user_approval);
    TEST_ASSERT_EQUAL(0, app_state.bytes_transferred);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dump_does_not_export_a_legacy_erased_nickname);
    RUN_TEST(test_dump_exports_the_whole_region);
    RUN_TEST(test_dump_exports_live_records_verbatim);
    RUN_TEST(test_dump_zeroes_the_slack_past_the_terminator);
    RUN_TEST(test_dump_refuses_a_store_that_does_not_parse);
    return UNITY_END();
}
