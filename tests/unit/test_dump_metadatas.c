#include <stdarg.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "handlers.h"
#include "io.h"
#include "metadata.h"
#include "types.h"
#include "ui.h"

// --- the symbols dump_metadatas.c and metadata.c resolve at link time -------

// globals.h declares `extern const internalStorage_t N_storage_real;` for the on-device
// layout (NVM is read-only from C's perspective; writes go through nvm_write). The test does
// not include globals.h so we can define the same symbol without `const` and let it land in
// writable BSS. The linker resolves the two declarations by symbol name; the qualifier
// mismatch is harmless.
internalStorage_t N_storage_real;
app_state_t app_state;
uint8_t G_io_apdu_buffer[IO_APDU_BUFFER_SIZE];

void nvm_write(void *dst, void *src, unsigned int len) {
    if (src == NULL) {
        memset(dst, 0, len);
    } else {
        memcpy(dst, src, len);
    }
}

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

int io_send_response_pointer(const uint8_t *ptr, size_t size, uint16_t sw) {
    assert_true(size >= TRANSFER_PAYLOAD_OFFSET);
    const size_t payload_size = size - TRANSFER_PAYLOAD_OFFSET;
    assert_true(sent_payload_len + payload_size <= sizeof(sent_payload));
    memcpy(&sent_payload[sent_payload_len], ptr + TRANSFER_PAYLOAD_OFFSET, payload_size);
    sent_payload_len += payload_size;
    responses_sent++;
    last_sw = sw;
    return (int) size;
}

int io_send_sw(uint16_t sw) {
    last_sw = sw;
    return 0;
}

void ui_idle(void) {
    idle_calls++;
}

void ui_request_user_approval(message_pair_t *msg) {
    (void) msg;
    approvals_requested++;
}

// --- helpers ----------------------------------------------------------------

// Append one live metadata entry holding `nickname` to N_storage_real.
static void add_password(const char *nickname) {
    const size_t name_len = strlen(nickname);
    const uint8_t datasize = 1 + name_len;  // charset byte + nickname bytes
    uint32_t offset = 0;
    while (N_storage_real.metadatas[offset] != 0) {
        offset += N_storage_real.metadatas[offset] + 2;
    }
    N_storage_real.metadatas[offset] = datasize;
    N_storage_real.metadatas[offset + 1] = META_NONE;
    N_storage_real.metadatas[offset + 2] = 0;  // charset byte
    memcpy(&N_storage_real.metadatas[offset + 3], nickname, name_len);
    // terminate the list so the parsing loops stop here
    N_storage_real.metadatas[offset + 3 + name_len] = 0;
    N_storage_real.metadata_count++;
}

static uint32_t raw_entry_offset(size_t nth) {
    uint32_t offset = 0;
    while (nth-- > 0) {
        assert_int_not_equal(N_storage_real.metadatas[offset], 0);
        offset += N_storage_real.metadatas[offset] + 2;
    }
    return offset;
}

// Mark the record at `offset` erased the way an app version that did not wipe the data block
// did: flip the kind byte, leave the nickname in flash. No current code path produces this --
// erase_metadata() wipes the block -- but a database carried across an upgrade contains such
// records, which is the case this file is about.
static void erase_metadata_legacy_style(uint32_t offset) {
    N_storage_real.metadatas[offset + 1] = META_ERASED;
    assert_true(N_storage_real.metadata_count > 0);
    N_storage_real.metadata_count--;
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
    assert_int_equal(dump_metadatas(), 0);
    // The first call only asks for approval; nothing may be exported before the user agrees.
    assert_int_equal(approvals_requested, 1);
    assert_int_equal(responses_sent, 0);

    app_state.user_approval = true;
    size_t chunks = 0;
    while (app_state.user_approval) {
        assert_int_equal(dump_metadatas(), 0);
        chunks++;
        // A stuck handler must not spin forever on a failed assertion elsewhere.
        assert_true(chunks <= sizeof(N_storage_real.metadatas));
    }
    return chunks;
}

static int setup(void **state __attribute__((unused))) {
    memset(&N_storage_real, 0, sizeof(N_storage_real));
    memset(&app_state, 0, sizeof(app_state));
    memset(G_io_apdu_buffer, 0, sizeof(G_io_apdu_buffer));
    memset(sent_payload, 0, sizeof(sent_payload));
    sent_payload_len = 0;
    responses_sent = 0;
    last_sw = 0;
    approvals_requested = 0;
    idle_calls = 0;
    return 0;
}

// --- tests ------------------------------------------------------------------

static void test_dump_does_not_export_a_legacy_erased_nickname(void **state
                                                               __attribute__((unused))) {
    // The exported region is bounded at the terminator, but a legacy erased record sits
    // *before* it, so its nickname was inside the bound and went out with the backup. The
    // handler compacts the store before the first chunk, which drops the record.
    add_password("alpha");
    add_password("secretsite");
    add_password("gamma");
    erase_metadata_legacy_style(raw_entry_offset(1));

    run_full_dump();

    assert_int_equal(last_sw, SWO_SUCCESS);
    assert_false(contains(sent_payload, sent_payload_len, "secretsite"));
    // The surviving names are still exported, so the backup is not simply empty.
    assert_true(contains(sent_payload, sent_payload_len, "alpha"));
    assert_true(contains(sent_payload, sent_payload_len, "gamma"));
}

static void test_dump_exports_the_whole_region(void **state __attribute__((unused))) {
    add_password("alpha");

    const size_t chunks = run_full_dump();

    // The host reads a fixed-size store, so the stream must cover it exactly whatever the
    // chunk size works out to.
    assert_int_equal(sent_payload_len, sizeof(N_storage_real.metadatas));
    assert_int_equal(chunks, responses_sent);
    assert_int_equal(idle_calls, 1);  // only the last chunk returns to the idle screen
}

static void test_dump_exports_live_records_verbatim(void **state __attribute__((unused))) {
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

    assert_memory_equal(sent_payload, expected, expected_len);
    // Everything past the terminator is zeroed rather than raw flash slack.
    for (size_t i = expected_len; i < sent_payload_len; i++) {
        assert_int_equal(sent_payload[i], 0);
    }
}

static void test_dump_zeroes_the_slack_past_the_terminator(void **state __attribute__((unused))) {
    add_password("alpha");
    // Stale bytes of a name that is no longer part of the database, sitting past its end.
    const char stale[] = "oldsecret";
    memcpy(&N_storage_real.metadatas[200], stale, sizeof(stale));

    run_full_dump();

    assert_false(contains(sent_payload, sent_payload_len, stale));
}

static void test_dump_refuses_a_store_that_does_not_parse(void **state __attribute__((unused))) {
    // A record claiming more data than MAX_METANAME allows: the store has no usable
    // terminator, so no length bound can be trusted and nothing may be exported.
    N_storage_real.metadatas[0] = MAX_METANAME + 1;
    N_storage_real.metadatas[1] = META_NONE;
    N_storage_real.metadata_count = 1;

    assert_int_equal(dump_metadatas(), 0);
    app_state.user_approval = true;
    assert_int_equal(dump_metadatas(), 0);

    assert_int_equal(last_sw, SW_METADATAS_PARSING_ERROR);
    assert_int_equal(responses_sent, 0);
    // The transfer is abandoned, not left half-open for the next command.
    assert_false(app_state.user_approval);
    assert_int_equal(app_state.bytes_transferred, 0);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dump_does_not_export_a_legacy_erased_nickname,
                                        setup,
                                        NULL),
        cmocka_unit_test_setup_teardown(test_dump_exports_the_whole_region, setup, NULL),
        cmocka_unit_test_setup_teardown(test_dump_exports_live_records_verbatim, setup, NULL),
        cmocka_unit_test_setup_teardown(test_dump_zeroes_the_slack_past_the_terminator,
                                        setup,
                                        NULL),
        cmocka_unit_test_setup_teardown(test_dump_refuses_a_store_that_does_not_parse, setup, NULL),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
