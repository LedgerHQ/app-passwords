#include <stdarg.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "metadata.h"
#include "password.h"
#include "types.h"

// --- stubs for symbols password.c / metadata.c reference at link time but
// --- which nickname_exists does not actually exercise.

// globals.h declares `extern const internalStorage_t N_storage_real;` for the
// on-device layout (NVM is read-only from C's perspective; writes go through
// nvm_write). The test does not include globals.h so we can define the same
// symbol without `const` and let it land in writable BSS. The linker resolves
// the two declarations by symbol name; the qualifier mismatch is harmless.
internalStorage_t N_storage_real;
uint8_t G_io_seproxyhal_spi_buffer[300];

void nvm_write(void *dst, void *src, unsigned int len) {
    if (src == NULL) {
        memset(dst, 0, len);
    } else {
        memcpy(dst, src, len);
    }
}

// password.c references these via type_password_at_offset / create_new_password,
// neither of which is called from nickname_exists. Stub them so the linker is
// happy.
bool type_password(uint8_t *seed,
                   size_t seed_size,
                   uint8_t *out_buffer,
                   uint8_t enabledSets,
                   const uint8_t *minSets,
                   size_t out_size) {
    (void) seed;
    (void) seed_size;
    (void) out_buffer;
    (void) enabledSets;
    (void) minSets;
    (void) out_size;
    return false;
}

uint8_t get_charset_options(void) {
    return 0;
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
    N_storage_real.metadatas[offset + 1] = 0;  // META_NONE -> live entry
    N_storage_real.metadatas[offset + 2] = 0;  // charset byte
    memcpy(&N_storage_real.metadatas[offset + 3], nickname, name_len);
    // terminate the list so get_metadata stops here
    N_storage_real.metadatas[offset + 3 + name_len] = 0;
    N_storage_real.metadata_count++;
}

static int setup(void **state __attribute__((unused))) {
    memset(&N_storage_real, 0, sizeof(N_storage_real));
    return 0;
}

// --- tests ------------------------------------------------------------------

static void test_nickname_exists_empty_db(void **state __attribute__((unused))) {
    assert_false(nickname_exists("foo", 3));
    assert_false(nickname_exists("", 0));
}

static void test_nickname_exists_found(void **state __attribute__((unused))) {
    add_password("alpha");
    add_password("beta");
    add_password("gamma");
    assert_true(nickname_exists("alpha", 5));
    assert_true(nickname_exists("beta", 4));
    assert_true(nickname_exists("gamma", 5));
}

static void test_nickname_exists_not_found(void **state __attribute__((unused))) {
    add_password("alpha");
    add_password("beta");
    assert_false(nickname_exists("delta", 5));
}

static void test_nickname_exists_length_mismatch(void **state __attribute__((unused))) {
    // A nickname stored as "alpha" must not match shorter or longer queries.
    add_password("alpha");
    assert_false(nickname_exists("alp", 3));
    assert_false(nickname_exists("alphabet", 8));
}

static void test_nickname_exists_case_sensitive(void **state __attribute__((unused))) {
    add_password("alpha");
    assert_false(nickname_exists("ALPHA", 5));
    assert_false(nickname_exists("Alpha", 5));
}

static void test_nickname_exists_truncation(void **state __attribute__((unused))) {
    // write_metadata clips the data block at MAX_METANAME bytes (charset +
    // nickname), so the longest persistable nickname is MAX_METANAME - 1 = 19
    // bytes. Anything longer is silently truncated. nickname_exists must
    // detect that a longer input would land on the same stored value, otherwise
    // a 20-char input could bypass the duplicate check.
    const char stored[] = "AAAAAAAAAAAAAAAAAAA";  // 19 chars
    assert_int_equal(strlen(stored), MAX_METANAME - 1);
    add_password(stored);

    // Exact 19-char match still works.
    assert_true(nickname_exists(stored, MAX_METANAME - 1));

    // 20 chars sharing the first 19 with the stored entry: truncation aware
    // duplicate detection must flag it as a conflict.
    const char twenty_chars_same_prefix[] = "AAAAAAAAAAAAAAAAAAAB";  // 20 chars
    assert_int_equal(strlen(twenty_chars_same_prefix), MAX_METANAME);
    assert_true(nickname_exists(twenty_chars_same_prefix, MAX_METANAME));
}

static void test_override_metadatas_high_offset(void **state __attribute__((unused))) {
    // Regression: override_metadatas() took the offset as a uint8_t, so offsets
    // >= 256 were truncated mod 256. A restore is streamed in 255-byte chunks,
    // so the second chunk onward landed at the wrong place and overwrote earlier
    // data. Writing at offset 300 must land at 300, not at 300 % 256 == 44.
    const uint8_t payload[] = {0xAA, 0xBB, 0xCC};
    override_metadatas(300, (void *) payload, sizeof(payload));

    assert_memory_equal(&N_storage_real.metadatas[300], payload, sizeof(payload));
    // The location it would have hit when truncated must be untouched.
    assert_int_equal(N_storage_real.metadatas[44], 0);
    assert_int_equal(N_storage_real.metadatas[45], 0);
    assert_int_equal(N_storage_real.metadatas[46], 0);
}

static void test_override_metadatas_buffer_end(void **state __attribute__((unused))) {
    // Writing the final bytes (offset + size == MAX_METADATAS) must be placed
    // correctly and stay within the buffer.
    const uint8_t payload[] = {0x11, 0x22, 0x33, 0x44};
    const size_t offset = MAX_METADATAS - sizeof(payload);
    override_metadatas(offset, (void *) payload, sizeof(payload));

    assert_memory_equal(&N_storage_real.metadatas[offset], payload, sizeof(payload));
}

static void test_write_metadata_enforces_capacity(void **state __attribute__((unused))) {
    // The store must refuse new entries once full and never report success past
    // the limit, so the 4096-byte buffer can never overflow.
    uint8_t name[MAX_METANAME];
    memset(name, 'A', sizeof(name));

    error_type_t err = OK;
    int written = 0;
    while ((err = write_metadata(name, sizeof(name))) == OK) {
        // Guard against an unbounded loop if the limit were not enforced.
        assert_true(++written < MAX_METADATAS);
    }

    assert_int_equal(err, ERR_NO_MORE_SPACE_AVAILABLE);
    assert_true(written > 0);
}

// Offset of the nth entry, walking the raw store rather than using get_metadata() (which
// skips erased entries) so the tests can assert on the physical layout.
static uint32_t raw_entry_offset(size_t nth) {
    uint32_t offset = 0;
    while (nth-- > 0) {
        assert_int_not_equal(N_storage_real.metadatas[offset], 0);
        offset += N_storage_real.metadatas[offset] + 2;
    }
    return offset;
}

static void assert_entry_is(uint32_t offset, const char *nickname) {
    const size_t name_len = strlen(nickname);
    assert_int_equal(N_storage_real.metadatas[offset], 1 + name_len);
    assert_int_equal(N_storage_real.metadatas[offset + 1], META_NONE);
    assert_memory_equal(&N_storage_real.metadatas[offset + 3], nickname, name_len);
}

static void test_compact_metadata_erased_first_entry(void **state __attribute__((unused))) {
    // Regression: compact_metadata() used shift_offset == 0 to mean "nothing erased yet",
    // but 0 is also where the live entries must move when the *first* entry is the erased
    // one. That conflation skipped compaction entirely and then counted the erased entry as
    // live, leaving metadata_count one too high after deleting the first password.
    add_password("alpha");
    add_password("beta");
    add_password("gamma");

    assert_int_equal(erase_metadata(0), OK);
    assert_int_equal(N_storage_real.metadata_count, 2);

    assert_int_equal(compact_metadata(), OK);

    assert_int_equal(N_storage_real.metadata_count, 2);
    assert_entry_is(raw_entry_offset(0), "beta");
    assert_entry_is(raw_entry_offset(1), "gamma");
    // No erased record may survive compaction.
    for (uint32_t offset = 0; N_storage_real.metadatas[offset] != 0;
         offset += N_storage_real.metadatas[offset] + 2) {
        assert_int_not_equal(N_storage_real.metadatas[offset + 1], META_ERASED);
    }
}

static void test_compact_metadata_erased_middle_entry(void **state __attribute__((unused))) {
    // The case that already worked, kept so the shift_offset rework cannot regress it.
    add_password("alpha");
    add_password("beta");
    add_password("gamma");

    assert_int_equal(erase_metadata(raw_entry_offset(1)), OK);
    assert_int_equal(compact_metadata(), OK);

    assert_int_equal(N_storage_real.metadata_count, 2);
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "gamma");
}

static void test_compact_metadata_no_erased_entry(void **state __attribute__((unused))) {
    // Compaction of a clean store must be a no-op, not a shift.
    add_password("alpha");
    add_password("beta");

    assert_int_equal(compact_metadata(), OK);

    assert_int_equal(N_storage_real.metadata_count, 2);
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "beta");
}

// True if `needle` appears anywhere in the raw metadata region.
static bool storage_contains(const char *needle) {
    const size_t len = strlen(needle);
    for (size_t i = 0; i + len <= sizeof(N_storage_real.metadatas); i++) {
        if (memcmp(&N_storage_real.metadatas[i], needle, len) == 0) {
            return true;
        }
    }
    return false;
}

static void test_deleted_nickname_is_wiped(void **state __attribute__((unused))) {
    // A deleted entry used to keep its nickname in flash: erase_metadata() only flipped the
    // kind byte, and compaction only moved a 2-byte terminator, so the name stayed readable in
    // the slack space and was handed out by the backup APDU.
    add_password("alpha");
    add_password("secretsite");
    add_password("gamma");
    assert_true(storage_contains("secretsite"));

    assert_int_equal(erase_metadata(raw_entry_offset(1)), OK);
    assert_false(storage_contains("secretsite"));

    assert_int_equal(compact_metadata(), OK);
    assert_false(storage_contains("secretsite"));
    // The surviving entries are intact.
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "gamma");
}

static void test_compaction_wipes_vacated_tail(void **state __attribute__((unused))) {
    // Compaction shifts the later entries down and then writes a 2-byte terminator at the new
    // end. Everything between that terminator and the old end of the database is stale data --
    // the tail of the entries that moved -- and must be zeroed, not left in the slack space.
    add_password("alpha");
    add_password("b");
    add_password("uniquetailnickname");
    const uint32_t old_end = raw_entry_offset(3);

    assert_int_equal(erase_metadata(raw_entry_offset(1)), OK);
    assert_int_equal(compact_metadata(), OK);

    const uint32_t new_end = find_free_metadata();
    assert_true(new_end < old_end);
    for (uint32_t i = new_end; i < old_end; i++) {
        assert_int_equal(N_storage_real.metadatas[i], 0);
    }

    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "uniquetailnickname");
    assert_int_equal(N_storage_real.metadata_count, 2);
}

// --- restore transaction (V-041) --------------------------------------------

static void test_restore_marker_lifecycle(void **state __attribute__((unused))) {
    assert_false(metadata_restore_in_progress());
    begin_metadata_restore();
    assert_true(metadata_restore_in_progress());
    end_metadata_restore();
    assert_false(metadata_restore_in_progress());
}

static void test_abort_metadata_restore_clears_database(void **state __attribute__((unused))) {
    // Half-written database: the old entries are still there, the first bytes come from the
    // new image, and nothing can tell them apart.
    add_password("alpha");
    add_password("beta");
    begin_metadata_restore();
    N_storage_real.metadatas[0] = 0xAA;
    N_storage_real.metadatas[1] = 0xBB;

    abort_metadata_restore();

    assert_false(metadata_restore_in_progress());
    assert_int_equal(N_storage_real.metadata_count, 0);
    for (size_t i = 0; i < sizeof(N_storage_real.metadatas); i++) {
        assert_int_equal(N_storage_real.metadatas[i], 0);
    }
    // The empty database parses cleanly.
    assert_int_equal(compact_metadata(), OK);
    assert_int_equal(get_metadata(0), UINT32_MAX);
}

// --- entry count capacity (V-006) -------------------------------------------

static void test_write_metadata_enforces_count_cap(void **state __attribute__((unused))) {
    // MAX_METADATAS fits far more short entries than the fixed-size UI list arrays can hold,
    // so creation has to stop at MAX_METADATA_COUNT and not just when the bytes run out.
    uint8_t name[2] = {0x07, 'a'};  // charset byte + 1-char nickname

    error_type_t err = OK;
    size_t written = 0;
    while ((err = write_metadata(name, sizeof(name))) == OK) {
        written++;
        assert_true(written <= MAX_METADATA_COUNT);
    }

    assert_int_equal(err, ERR_NO_MORE_SPACE_AVAILABLE);
    assert_int_equal(written, MAX_METADATA_COUNT);
    assert_int_equal(N_storage_real.metadata_count, MAX_METADATA_COUNT);
}

static void test_compact_metadata_rejects_over_count_database(void **state
                                                              __attribute__((unused))) {
    // A restored image can hold more short entries than the UI can address; the count must be
    // rejected rather than committed to NVM.
    memset(N_storage_real.metadatas, 0, sizeof(N_storage_real.metadatas));
    const size_t entries = MAX_METADATA_COUNT + 1;
    for (size_t i = 0; i < entries; i++) {
        N_storage_real.metadatas[i * 4] = 2;  // 4 bytes per record
        N_storage_real.metadatas[i * 4 + 1] = META_NONE;
    }
    N_storage_real.metadata_count = 0;

    assert_int_equal(compact_metadata(), ERR_NO_MORE_SPACE_AVAILABLE);
    // Nothing was committed.
    assert_int_equal(N_storage_real.metadata_count, 0);
}

// --- bounded parser (V-008) -------------------------------------------------

static void test_compact_metadata_rejects_missing_terminator(void **state __attribute__((unused))) {
    // A database whose records tile the whole array with no zero terminator used to walk the
    // parser past the end: the loop read METADATA_DATALEN(offset) before testing the bound.
    memset(N_storage_real.metadatas, 0, sizeof(N_storage_real.metadatas));
    for (size_t offset = 0; offset < sizeof(N_storage_real.metadatas); offset += 4) {
        N_storage_real.metadatas[offset] = 2;         // datalen: charset + 1 byte
        N_storage_real.metadatas[offset + 1] = 0x00;  // META_NONE
    }

    assert_int_equal(compact_metadata(), ERR_CORRUPTED_METADATA);
}

static void test_compact_metadata_rejects_record_overrunning_the_end(void **state
                                                                     __attribute__((unused))) {
    // A small, apparently valid record placed near the end of the array, declaring a payload
    // that runs past MAX_METADATAS. The parser has to walk there, so tile the space before it.
    memset(N_storage_real.metadatas, 0, sizeof(N_storage_real.metadatas));
    const size_t last = sizeof(N_storage_real.metadatas) - 4;
    for (size_t offset = 0; offset < last; offset += 4) {
        N_storage_real.metadatas[offset] = 2;  // datalen: charset + 1 byte -> 4 bytes total
        N_storage_real.metadatas[offset + 1] = META_NONE;
    }
    // Only 4 bytes are left here, but this record claims MAX_METANAME + 2 == 22.
    N_storage_real.metadatas[last] = MAX_METANAME;
    N_storage_real.metadatas[last + 1] = META_NONE;

    assert_int_equal(compact_metadata(), ERR_CORRUPTED_METADATA);
}

static void test_compact_metadata_rejects_bad_kind(void **state __attribute__((unused))) {
    add_password("alpha");
    N_storage_real.metadatas[1] = 0x42;  // neither META_NONE nor META_ERASED

    assert_int_equal(compact_metadata(), ERR_CORRUPTED_METADATA);
}

static void test_compact_metadata_rejects_oversized_entry(void **state __attribute__((unused))) {
    // Preserved behaviour: an entry longer than a full nickname is reported as too big, which
    // the restore path maps to SW_METADATAS_PARSING_ERROR.
    memset(N_storage_real.metadatas, 0, sizeof(N_storage_real.metadatas));
    N_storage_real.metadatas[0] = MAX_METANAME + 1;
    N_storage_real.metadatas[1] = 0x00;

    assert_int_equal(compact_metadata(), ERR_METADATA_ENTRY_TOO_BIG);
}

static void test_get_metadata_terminates_on_corrupt_storage(void **state __attribute__((unused))) {
    // get_metadata() used to loop forever on a database with no terminator.
    memset(N_storage_real.metadatas, 0xFF, sizeof(N_storage_real.metadatas));

    assert_int_equal(get_metadata(0), UINT32_MAX);
    assert_int_equal(find_free_metadata(), MAX_METADATAS);
}

static void test_erase_metadata_rejects_out_of_range_offset(void **state __attribute__((unused))) {
    add_password("alpha");

    assert_int_equal(erase_metadata(MAX_METADATAS), ERR_CORRUPTED_METADATA);
    assert_int_equal(erase_metadata(MAX_METADATAS - 1), ERR_CORRUPTED_METADATA);
    // The live entry is untouched.
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_int_equal(N_storage_real.metadata_count, 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_nickname_exists_empty_db, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_found, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_not_found, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_length_mismatch, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_case_sensitive, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_truncation, setup, NULL),
        cmocka_unit_test_setup_teardown(test_override_metadatas_high_offset, setup, NULL),
        cmocka_unit_test_setup_teardown(test_override_metadatas_buffer_end, setup, NULL),
        cmocka_unit_test_setup_teardown(test_write_metadata_enforces_capacity, setup, NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_erased_first_entry, setup, NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_erased_middle_entry, setup, NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_no_erased_entry, setup, NULL),
        cmocka_unit_test_setup_teardown(test_deleted_nickname_is_wiped, setup, NULL),
        cmocka_unit_test_setup_teardown(test_compaction_wipes_vacated_tail, setup, NULL),
        cmocka_unit_test_setup_teardown(test_restore_marker_lifecycle, setup, NULL),
        cmocka_unit_test_setup_teardown(test_abort_metadata_restore_clears_database, setup, NULL),
        cmocka_unit_test_setup_teardown(test_write_metadata_enforces_count_cap, setup, NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_rejects_over_count_database,
                                        setup,
                                        NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_rejects_missing_terminator,
                                        setup,
                                        NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_rejects_record_overrunning_the_end,
                                        setup,
                                        NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_rejects_bad_kind, setup, NULL),
        cmocka_unit_test_setup_teardown(test_compact_metadata_rejects_oversized_entry, setup, NULL),
        cmocka_unit_test_setup_teardown(test_get_metadata_terminates_on_corrupt_storage,
                                        setup,
                                        NULL),
        cmocka_unit_test_setup_teardown(test_erase_metadata_rejects_out_of_range_offset,
                                        setup,
                                        NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
