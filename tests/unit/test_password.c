#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "metadata.h"
#include "password.h"
#include "types.h"
#include "fake_vars.h"
#include "Mockoptions.h"
#include "Mockpassword_typing.h"

// Settings read by create_new_password()
static uint8_t stub_charsets = 0;
static uint8_t stub_password_length = 32;

static uint8_t fake_get_charset_options(int cmock_num_calls) {
    (void) cmock_num_calls;
    return stub_charsets;
}

static uint8_t fake_get_password_length(int cmock_num_calls) {
    (void) cmock_num_calls;
    return stub_password_length;
}

// Arguments of the last type_password() call, and its result
static struct {
    size_t calls;
    const uint8_t *data;
    uint32_t data_size;
    const uint8_t *out;
    setmask_t sets;
    uint8_t version;
    uint32_t size;
} typed;
static bool type_result;

static bool fake_type_password(uint8_t *data,
                               uint32_t dataSize,
                               uint8_t *out,
                               setmask_t setMask,
                               uint8_t version,
                               uint32_t size,
                               int cmock_num_calls) {
    (void) cmock_num_calls;
    typed.calls++;
    typed.data = data;
    typed.data_size = dataSize;
    typed.out = out;
    typed.sets = setMask;
    typed.version = version;
    typed.size = size;
    return type_result;
}

// --- helpers ----------------------------------------------------------------

// Append one live metadata entry holding `nickname` to N_storage_rw.
static void add_password(const char *nickname) {
    const size_t name_len = strlen(nickname);
    const uint8_t datasize = 1 + name_len;  // charset byte + nickname bytes
    uint32_t offset = 0;
    while (N_storage_rw.metadatas[offset] != 0) {
        offset += N_storage_rw.metadatas[offset] + 2;
    }
    N_storage_rw.metadatas[offset] = datasize;
    N_storage_rw.metadatas[offset + 1] = 0;  // META_NONE -> live entry
    N_storage_rw.metadatas[offset + 2] = 0;  // charset byte
    memcpy(&N_storage_rw.metadatas[offset + 3], nickname, name_len);
    // terminate the list so get_metadata stops here
    N_storage_rw.metadatas[offset + 3 + name_len] = 0;
    N_storage_rw.metadata_count++;
}

void setUp(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
    stub_charsets = 0;
    stub_password_length = 32;
    Mockoptions_Init();
    Mockpassword_typing_Init();
    get_charset_options_StubWithCallback(fake_get_charset_options);
    get_password_length_StubWithCallback(fake_get_password_length);
    memset(&typed, 0, sizeof(typed));
    type_result = true;
    type_password_StubWithCallback(fake_type_password);
}

void tearDown(void) {
    Mockoptions_Verify();
    Mockpassword_typing_Verify();
    Mockoptions_Destroy();
    Mockpassword_typing_Destroy();
}

// --- tests ------------------------------------------------------------------

static void test_nickname_exists_empty_db(void) {
    TEST_ASSERT_FALSE(nickname_exists("foo", 3));
    TEST_ASSERT_FALSE(nickname_exists("", 0));
}

static void test_nickname_exists_found(void) {
    add_password("alpha");
    add_password("beta");
    add_password("gamma");
    TEST_ASSERT_TRUE(nickname_exists("alpha", 5));
    TEST_ASSERT_TRUE(nickname_exists("beta", 4));
    TEST_ASSERT_TRUE(nickname_exists("gamma", 5));
}

static void test_nickname_exists_not_found(void) {
    add_password("alpha");
    add_password("beta");
    TEST_ASSERT_FALSE(nickname_exists("delta", 5));
}

static void test_nickname_exists_length_mismatch(void) {
    // A nickname stored as "alpha" must not match shorter or longer queries.
    add_password("alpha");
    TEST_ASSERT_FALSE(nickname_exists("alp", 3));
    TEST_ASSERT_FALSE(nickname_exists("alphabet", 8));
}

static void test_nickname_exists_case_sensitive(void) {
    add_password("alpha");
    TEST_ASSERT_FALSE(nickname_exists("ALPHA", 5));
    TEST_ASSERT_FALSE(nickname_exists("Alpha", 5));
}

static void test_nickname_exists_truncation(void) {
    // The longest persistable nickname is MAX_NICKNAME_LEN = 19 bytes, and
    // older entries could hold a truncated longer one. nickname_exists must
    // detect that a longer input would land on the same stored value, otherwise
    // a 20-char input could bypass the duplicate check.
    const char stored[] = "AAAAAAAAAAAAAAAAAAA";  // 19 chars
    TEST_ASSERT_EQUAL(MAX_NICKNAME_LEN, strlen(stored));
    add_password(stored);

    // Exact 19-char match still works.
    TEST_ASSERT_TRUE(nickname_exists(stored, MAX_NICKNAME_LEN));

    // 20 chars sharing the first 19 with the stored entry: truncation aware
    // duplicate detection must flag it as a conflict.
    const char twenty_chars_same_prefix[] = "AAAAAAAAAAAAAAAAAAAB";  // 20 chars
    TEST_ASSERT_EQUAL(MAX_NICKNAME_LEN + 1, strlen(twenty_chars_same_prefix));
    TEST_ASSERT_TRUE(nickname_exists(twenty_chars_same_prefix, MAX_NICKNAME_LEN + 1));
}

static void test_create_new_password_rejects_an_untruncatable_nickname(void) {
    // Only MAX_NICKNAME_LEN nickname bytes fit. write_metadata() clamps the block silently, so a
    // longer nickname used to be accepted and then stored short -- the entry no longer matched
    // what the caller asked for, and since the nickname is the derivation seed, the password
    // generated from it was not the one the user thought they had created.
    char too_long[MAX_NICKNAME_LEN + 2];
    memset(too_long, 'A', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';
    TEST_ASSERT_EQUAL(MAX_NICKNAME_LEN + 1, strlen(too_long));

    TEST_ASSERT_EQUAL(ERR_METADATA_ENTRY_TOO_BIG, create_new_password(too_long, strlen(too_long)));
    // Nothing was persisted: no entry, and the store is still empty.
    TEST_ASSERT_EQUAL(0, N_storage_rw.metadata_count);
    TEST_ASSERT_EQUAL(0, N_storage_rw.metadatas[0]);
}

static void test_create_new_password_accepts_the_longest_storable_nickname(void) {
    char longest[MAX_NICKNAME_LEN + 1];
    memset(longest, 'A', sizeof(longest) - 1);
    longest[sizeof(longest) - 1] = '\0';

    TEST_ASSERT_EQUAL(OK, create_new_password(longest, strlen(longest)));
    TEST_ASSERT_EQUAL(1, N_storage_rw.metadata_count);
    // Stored whole, not clamped: the data block is the charset and length bytes plus every
    // nickname byte.
    TEST_ASSERT_EQUAL(2 + MAX_NICKNAME_LEN, N_storage_rw.metadatas[0]);
    // Nickname bytes start after the datalen, kind, charset and length bytes.
    TEST_ASSERT_EQUAL_MEMORY(longest, &N_storage_rw.metadatas[4], MAX_NICKNAME_LEN);
    TEST_ASSERT_EQUAL(MAX_NICKNAME_LEN, METADATA_NICKNAME_LEN(0));
}

static void test_create_new_password_writes_the_current_version(void) {
    TEST_ASSERT_EQUAL(OK, create_new_password("mail", 4));
    TEST_ASSERT_EQUAL(META_CURRENT_VERSION, N_storage_rw.metadatas[1]);
    // The password length comes from the setting.
    TEST_ASSERT_EQUAL(stub_password_length, N_storage_rw.metadatas[3]);
    TEST_ASSERT_EQUAL(stub_password_length, METADATA_PASSWORD_LEN(0));
    TEST_ASSERT_EQUAL_MEMORY("mail", METADATA_NICKNAME(0), 4);
    TEST_ASSERT_EQUAL(4, METADATA_NICKNAME_LEN(0));
}

static void test_version_0_entry_accessors(void) {
    add_password("legacy");
    TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, METADATA_PASSWORD_LEN(0));
    TEST_ASSERT_EQUAL_MEMORY("legacy", METADATA_NICKNAME(0), 6);
    TEST_ASSERT_EQUAL(6, METADATA_NICKNAME_LEN(0));
}

static void test_validate_metadata_image_lengths(void) {
    uint8_t image[MAX_METADATAS] = {0};

    // A new entry with a supported length and the longest nickname.
    image[0] = 2 + MAX_NICKNAME_LEN;
    image[1] = META_CURRENT_VERSION;
    image[2] = 0x07;
    image[3] = 64;
    memset(&image[4], 'A', MAX_NICKNAME_LEN);
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));

    // Unsupported lengths.
    image[3] = 0;
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));
    image[3] = 65;
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));

    // No room for the length byte.
    memset(image, 0, sizeof(image));
    image[0] = 1;
    image[1] = META_CURRENT_VERSION;
    image[2] = 0x07;
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));

    // A version 0 entry cannot hold a 20-byte nickname.
    memset(image, 0, sizeof(image));
    image[0] = 2 + MAX_NICKNAME_LEN;
    image[1] = META_VERSION_0;
    TEST_ASSERT_EQUAL(ERR_METADATA_ENTRY_TOO_BIG, validate_metadata_image(image));
}

static void test_validate_full_version_0_database(void) {
    // As many version 0 entries with the longest nickname as before: any existing backup fits.
    uint8_t image[MAX_METADATAS] = {0};
    uint32_t offset = 0;
    for (size_t i = 0; i < MAX_METADATA_COUNT; i++) {
        image[offset] = 1 + MAX_NICKNAME_LEN;
        image[offset + 1] = META_VERSION_0;
        image[offset + 2] = 0x07;
        memset(&image[offset + 3], 'A', MAX_NICKNAME_LEN);
        offset += 3 + MAX_NICKNAME_LEN;
    }
    TEST_ASSERT_TRUE(offset + 2 <= MAX_METADATAS);
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));
}

static void test_validate_metadata_image_versions(void) {
    uint8_t image[MAX_METADATAS] = {0};

    // One version 0 entry and one version 1 entry.
    const uint8_t valid[] = {0x02, META_VERSION_0, 0x07, 'a', 0x03, META_VERSION_1, 0xFF, 20, 'b'};
    memcpy(image, valid, sizeof(valid));
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));

    // A version this app does not know.
    image[5] = META_CURRENT_VERSION + 1;
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));
}

static void test_compaction_keeps_the_entry_version(void) {
    add_password("alpha");
    // A version 1 entry, moved down when "alpha" is erased.
    TEST_ASSERT_EQUAL(OK, create_new_password("beta", 4));
    TEST_ASSERT_EQUAL(OK, erase_metadata(get_metadata(0)));

    TEST_ASSERT_EQUAL(OK, compact_metadata());
    TEST_ASSERT_EQUAL(2 + strlen("beta"), N_storage_rw.metadatas[0]);
    TEST_ASSERT_EQUAL(META_VERSION_1, N_storage_rw.metadatas[1]);
    TEST_ASSERT_EQUAL(stub_password_length, N_storage_rw.metadatas[3]);
}

static void test_validate_metadata_image_nicknames(void) {
    uint8_t image[MAX_METADATAS] = {0};
    add_password("before");
    const internalStorage_t before = N_storage_rw;

    // Printable nicknames of 1 to MAX_NICKNAME_LEN bytes, in both layouts.
    const uint8_t valid[] = {0x02, META_VERSION_0, 0x07, ' ', 0x03, META_VERSION_1, 0x07, 20, '~'};
    memcpy(image, valid, sizeof(valid));
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));

    // Hidden byte after a displayed prefix: "foo\0x".
    const uint8_t hidden[] = {0x06, META_VERSION_0, 0x07, 'f', 'o', 'o', 0x00, 'x'};
    memset(image, 0, sizeof(image));
    memcpy(image, hidden, sizeof(hidden));
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));

    // Control character, DEL, and non-ASCII bytes.
    const uint8_t bad_bytes[] = {0x1F, 0x7F, 0x80, 0xFF};
    for (size_t i = 0; i < sizeof(bad_bytes); i++) {
        const uint8_t entry[] = {0x03, META_VERSION_1, 0x07, 20, bad_bytes[i]};
        memset(image, 0, sizeof(image));
        memcpy(image, entry, sizeof(entry));
        TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));
    }

    // Empty nicknames, in both layouts.
    const uint8_t empty_v0[] = {0x01, META_VERSION_0, 0x07};
    memset(image, 0, sizeof(image));
    memcpy(image, empty_v0, sizeof(empty_v0));
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));
    const uint8_t empty_v1[] = {0x02, META_VERSION_1, 0x07, 20};
    memset(image, 0, sizeof(image));
    memcpy(image, empty_v1, sizeof(empty_v1));
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));

    // An erased record keeps its wiped data block.
    const uint8_t erased[] =
        {0x04, META_ERASED, 0x00, 0x00, 0x00, 0x00, 0x02, META_VERSION_0, 0x07, 'a'};
    memset(image, 0, sizeof(image));
    memcpy(image, erased, sizeof(erased));
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));

    // Validation never writes to the live database.
    TEST_ASSERT_EQUAL_MEMORY(&before, &N_storage_rw, sizeof(before));
}

static void test_charsets_are_weak(void) {
    // Separators only, together or alone.
    TEST_ASSERT_TRUE(charsets_are_weak(0x38));
    TEST_ASSERT_TRUE(charsets_are_weak(0x08));
    TEST_ASSERT_TRUE(charsets_are_weak(0x10));
    TEST_ASSERT_TRUE(charsets_are_weak(0x20));
    // Zero means all the sets; any other set makes the alphabet acceptable.
    TEST_ASSERT_FALSE(charsets_are_weak(0x00));
    TEST_ASSERT_FALSE(charsets_are_weak(0x39));
    TEST_ASSERT_FALSE(charsets_are_weak(0x78));
    TEST_ASSERT_FALSE(charsets_are_weak(0x04));
}

static void test_create_new_password_refuses_weak_charsets(void) {
    stub_charsets = 0x38;
    TEST_ASSERT_EQUAL(ERR_WEAK_CHARSETS, create_new_password("mail", 4));
    TEST_ASSERT_EQUAL(0, N_storage_rw.metadata_count);
    TEST_ASSERT_EQUAL(0, N_storage_rw.metadatas[0]);

    stub_charsets = 0x3C;
    TEST_ASSERT_EQUAL(OK, create_new_password("mail", 4));
}

static void test_validate_metadata_image_weak_charsets(void) {
    uint8_t image[MAX_METADATAS] = {0};

    // A version 0 entry keeps its weak charset: its password may be in use.
    const uint8_t legacy[] = {0x02, META_VERSION_0, 0x08, 'a'};
    memcpy(image, legacy, sizeof(legacy));
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));

    // A newer entry cannot have one.
    const uint8_t weak[] = {0x03, META_CURRENT_VERSION, 0x38, 20, 'a'};
    memset(image, 0, sizeof(image));
    memcpy(image, weak, sizeof(weak));
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));
}

static void test_password_length_fits_charsets(void) {
    // PIN lengths take numbers only; zero means all the sets, so it does not fit them.
    TEST_ASSERT_TRUE(password_length_fits_charsets(4, NUMBERS_SET));
    TEST_ASSERT_TRUE(password_length_fits_charsets(6, NUMBERS_SET));
    TEST_ASSERT_FALSE(password_length_fits_charsets(4, 0x07));
    TEST_ASSERT_FALSE(password_length_fits_charsets(6, 0x00));
    // Other lengths take any charset, numbers only included.
    TEST_ASSERT_TRUE(password_length_fits_charsets(8, 0xFF));
    TEST_ASSERT_TRUE(password_length_fits_charsets(20, NUMBERS_SET));
}

static void test_create_new_password_pin_needs_numbers(void) {
    stub_password_length = 4;
    stub_charsets = 0x07;
    TEST_ASSERT_EQUAL(ERR_PIN_NEEDS_NUMBERS, create_new_password("pin", 3));
    TEST_ASSERT_EQUAL(0, N_storage_rw.metadata_count);

    stub_charsets = NUMBERS_SET;
    TEST_ASSERT_EQUAL(OK, create_new_password("pin", 3));
    TEST_ASSERT_EQUAL(4, METADATA_PASSWORD_LEN(0));
}

static void test_validate_metadata_image_pin_lengths(void) {
    uint8_t image[MAX_METADATAS] = {0};

    const uint8_t pin[] = {0x05, META_CURRENT_VERSION, NUMBERS_SET, 6, 'b', 'a', 'n'};
    memcpy(image, pin, sizeof(pin));
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));

    // A PIN length with other characters, or with all the sets.
    image[2] = 0x07;
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));
    image[2] = 0x00;
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, validate_metadata_image(image));
}

static void test_validate_metadata_image(void) {
    uint8_t image[MAX_METADATAS] = {0};
    add_password("before");
    const internalStorage_t before = N_storage_rw;

    // Entries "a" (live) and "allah" (erased), then the terminator.
    const uint8_t valid[] =
        {0x02, META_NONE, 0x07, 'a', 0x06, META_ERASED, 0x07, 'a', 'l', 'l', 'a', 'h'};
    memcpy(image, valid, sizeof(valid));
    TEST_ASSERT_EQUAL(OK, validate_metadata_image(image));

    // A record claiming more data than a nickname holds.
    image[0] = MAX_METANAME + 1;
    TEST_ASSERT_EQUAL(ERR_METADATA_ENTRY_TOO_BIG, validate_metadata_image(image));

    // No terminator before the end of the image.
    memset(image, 0xFF, sizeof(image));
    TEST_ASSERT_NOT_EQUAL(OK, validate_metadata_image(image));

    // More live entries than the password list can address.
    memset(image, 0, sizeof(image));
    for (size_t i = 0; i <= MAX_METADATA_COUNT; i++) {
        image[4 * i] = 2;
        image[4 * i + 2] = 0x07;
        image[4 * i + 3] = 'a';
    }
    TEST_ASSERT_EQUAL(ERR_NO_MORE_SPACE_AVAILABLE, validate_metadata_image(image));

    // Validation never writes to the live database.
    TEST_ASSERT_EQUAL_MEMORY(&before, &N_storage_rw, sizeof(before));
}

static void test_commit_metadata_image(void) {
    uint8_t image[MAX_METADATAS] = {0};
    add_password("before");

    const uint8_t erased_then_live[] =
        {0x02, META_ERASED, 0x07, 'a', 0x06, META_NONE, 0x07, 'a', 'l', 'l', 'a', 'h'};
    memcpy(image, erased_then_live, sizeof(erased_then_live));
    TEST_ASSERT_EQUAL(OK, commit_metadata_image(image));

    // The previous database is gone, the erased entry compacted away.
    const uint8_t expected[] = {0x06, META_NONE, 0x07, 'a', 'l', 'l', 'a', 'h', 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, N_storage_rw.metadatas, sizeof(expected));
    for (size_t i = sizeof(expected); i < MAX_METADATAS; i++) {
        TEST_ASSERT_EQUAL(0, N_storage_rw.metadatas[i]);
    }
    TEST_ASSERT_EQUAL(1, N_storage_rw.metadata_count);
    TEST_ASSERT_FALSE(metadata_restore_in_progress());
}

static void test_write_metadata_enforces_capacity(void) {
    // The store must refuse new entries once full and never report success past
    // the limit, so the 4096-byte buffer can never overflow.
    uint8_t name[MAX_METANAME];
    memset(name, 'A', sizeof(name));
    name[0] = 0x07;                   // charset
    name[1] = PASSWORD_DEFAULT_SIZE;  // password length

    error_type_t err = OK;
    int written = 0;
    while ((err = write_metadata(name, sizeof(name), META_CURRENT_VERSION)) == OK) {
        // Guard against an unbounded loop if the limit were not enforced.
        TEST_ASSERT_TRUE(++written < MAX_METADATAS);
    }

    TEST_ASSERT_EQUAL(ERR_NO_MORE_SPACE_AVAILABLE, err);
    TEST_ASSERT_TRUE(written > 0);
}

// Offset of the nth entry, walking the raw store rather than using get_metadata() (which
// skips erased entries) so the tests can assert on the physical layout.
static uint32_t raw_entry_offset(size_t nth) {
    uint32_t offset = 0;
    while (nth-- > 0) {
        TEST_ASSERT_NOT_EQUAL(0, N_storage_rw.metadatas[offset]);
        offset += N_storage_rw.metadatas[offset] + 2;
    }
    return offset;
}

static void assert_entry_is(uint32_t offset, const char *nickname) {
    const size_t name_len = strlen(nickname);
    TEST_ASSERT_EQUAL(1 + name_len, N_storage_rw.metadatas[offset]);
    TEST_ASSERT_EQUAL(META_NONE, N_storage_rw.metadatas[offset + 1]);
    TEST_ASSERT_EQUAL_MEMORY(nickname, &N_storage_rw.metadatas[offset + 3], name_len);
}

static void test_compact_metadata_erased_first_entry(void) {
    // Regression: compact_metadata() used shift_offset == 0 to mean "nothing erased yet",
    // but 0 is also where the live entries must move when the *first* entry is the erased
    // one. That conflation skipped compaction entirely and then counted the erased entry as
    // live, leaving metadata_count one too high after deleting the first password.
    add_password("alpha");
    add_password("beta");
    add_password("gamma");

    TEST_ASSERT_EQUAL(OK, erase_metadata(0));
    TEST_ASSERT_EQUAL(2, N_storage_rw.metadata_count);

    TEST_ASSERT_EQUAL(OK, compact_metadata());

    TEST_ASSERT_EQUAL(2, N_storage_rw.metadata_count);
    assert_entry_is(raw_entry_offset(0), "beta");
    assert_entry_is(raw_entry_offset(1), "gamma");
    // No erased record may survive compaction.
    for (uint32_t offset = 0; N_storage_rw.metadatas[offset] != 0;
         offset += N_storage_rw.metadatas[offset] + 2) {
        TEST_ASSERT_NOT_EQUAL(META_ERASED, N_storage_rw.metadatas[offset + 1]);
    }
}

static void test_compact_metadata_erased_middle_entry(void) {
    // The case that already worked, kept so the shift_offset rework cannot regress it.
    add_password("alpha");
    add_password("beta");
    add_password("gamma");

    TEST_ASSERT_EQUAL(OK, erase_metadata(raw_entry_offset(1)));
    TEST_ASSERT_EQUAL(OK, compact_metadata());

    TEST_ASSERT_EQUAL(2, N_storage_rw.metadata_count);
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "gamma");
}

static void test_compact_metadata_no_erased_entry(void) {
    // Compaction of a clean store must be a no-op, not a shift.
    add_password("alpha");
    add_password("beta");

    TEST_ASSERT_EQUAL(OK, compact_metadata());

    TEST_ASSERT_EQUAL(2, N_storage_rw.metadata_count);
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "beta");
}

// True if `needle` appears anywhere in the raw metadata region.
static bool storage_contains(const char *needle) {
    const size_t len = strlen(needle);
    for (size_t i = 0; i + len <= sizeof(N_storage_rw.metadatas); i++) {
        if (memcmp(&N_storage_rw.metadatas[i], needle, len) == 0) {
            return true;
        }
    }
    return false;
}

static void test_deleted_nickname_is_wiped(void) {
    // A deleted entry used to keep its nickname in flash: erase_metadata() only flipped the
    // kind byte, and compaction only moved a 2-byte terminator, so the name stayed readable in
    // the slack space and was handed out by the backup APDU.
    add_password("alpha");
    add_password("secretsite");
    add_password("gamma");
    TEST_ASSERT_TRUE(storage_contains("secretsite"));

    TEST_ASSERT_EQUAL(OK, erase_metadata(raw_entry_offset(1)));
    TEST_ASSERT_FALSE(storage_contains("secretsite"));

    TEST_ASSERT_EQUAL(OK, compact_metadata());
    TEST_ASSERT_FALSE(storage_contains("secretsite"));
    // The surviving entries are intact.
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "gamma");
}

// Mark the record at `offset` erased the way an older app version did: flip the kind byte and
// leave the nickname in place. No current code path can produce this -- erase_metadata() wipes
// the data block -- but a database carried across an upgrade still contains such records.
static void erase_metadata_legacy_style(uint32_t offset) {
    const unsigned char m = META_ERASED;
    nvm_write((void *) &N_storage_rw.metadatas[offset + 1], (void *) &m, 1);
    TEST_ASSERT_TRUE(N_storage_rw.metadata_count > 0);
    N_storage_rw.metadata_count--;
}

static void test_compaction_removes_a_legacy_erased_nickname(void) {
    // dump_metadatas() bounds the exported region at the terminator, but a legacy erased record
    // sits *before* it, so its nickname counted as live and was handed to the backup. The dump
    // compacts first; compaction must leave no trace of the name.
    add_password("alpha");
    add_password("secretsite");
    add_password("gamma");

    erase_metadata_legacy_style(raw_entry_offset(1));
    // The point of the scenario: the nickname is still there after the deletion.
    TEST_ASSERT_TRUE(storage_contains("secretsite"));

    TEST_ASSERT_EQUAL(OK, compact_metadata());

    TEST_ASSERT_FALSE(storage_contains("secretsite"));
    TEST_ASSERT_EQUAL(2, N_storage_rw.metadata_count);
    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "gamma");
}

static void test_compaction_wipes_vacated_tail(void) {
    // Compaction shifts the later entries down and then writes a 2-byte terminator at the new
    // end. Everything between that terminator and the old end of the database is stale data --
    // the tail of the entries that moved -- and must be zeroed, not left in the slack space.
    add_password("alpha");
    add_password("b");
    add_password("uniquetailnickname");
    const uint32_t old_end = raw_entry_offset(3);

    TEST_ASSERT_EQUAL(OK, erase_metadata(raw_entry_offset(1)));
    TEST_ASSERT_EQUAL(OK, compact_metadata());

    const uint32_t new_end = find_free_metadata();
    TEST_ASSERT_TRUE(new_end < old_end);
    for (uint32_t i = new_end; i < old_end; i++) {
        TEST_ASSERT_EQUAL(0, N_storage_rw.metadatas[i]);
    }

    assert_entry_is(raw_entry_offset(0), "alpha");
    assert_entry_is(raw_entry_offset(1), "uniquetailnickname");
    TEST_ASSERT_EQUAL(2, N_storage_rw.metadata_count);
}

// --- restore transaction (V-041) --------------------------------------------

static void test_restore_marker_lifecycle(void) {
    TEST_ASSERT_FALSE(metadata_restore_in_progress());
    begin_metadata_restore();
    TEST_ASSERT_TRUE(metadata_restore_in_progress());
    end_metadata_restore();
    TEST_ASSERT_FALSE(metadata_restore_in_progress());
}

static void test_abort_metadata_restore_clears_database(void) {
    // Half-written database: the old entries are still there, the first bytes come from the
    // new image, and nothing can tell them apart.
    add_password("alpha");
    add_password("beta");
    begin_metadata_restore();
    N_storage_rw.metadatas[0] = 0xAA;
    N_storage_rw.metadatas[1] = 0xBB;

    abort_metadata_restore();

    TEST_ASSERT_FALSE(metadata_restore_in_progress());
    TEST_ASSERT_EQUAL(0, N_storage_rw.metadata_count);
    for (size_t i = 0; i < sizeof(N_storage_rw.metadatas); i++) {
        TEST_ASSERT_EQUAL(0, N_storage_rw.metadatas[i]);
    }
    // The empty database parses cleanly.
    TEST_ASSERT_EQUAL(OK, compact_metadata());
    TEST_ASSERT_EQUAL(UINT32_MAX, get_metadata(0));
}

// --- entry count capacity (V-006) -------------------------------------------

static void test_write_metadata_enforces_count_cap(void) {
    // MAX_METADATAS fits far more short entries than the fixed-size UI list arrays can hold,
    // so creation has to stop at MAX_METADATA_COUNT and not just when the bytes run out.
    uint8_t name[3] = {0x07, PASSWORD_DEFAULT_SIZE, 'a'};  // charset, length, 1-char nickname

    error_type_t err = OK;
    size_t written = 0;
    while ((err = write_metadata(name, sizeof(name), META_CURRENT_VERSION)) == OK) {
        written++;
        TEST_ASSERT_TRUE(written <= MAX_METADATA_COUNT);
    }

    TEST_ASSERT_EQUAL(ERR_NO_MORE_SPACE_AVAILABLE, err);
    TEST_ASSERT_EQUAL(MAX_METADATA_COUNT, written);
    TEST_ASSERT_EQUAL(MAX_METADATA_COUNT, N_storage_rw.metadata_count);
}

static void test_compact_metadata_rejects_over_count_database(void) {
    // A restored image can hold more short entries than the UI can address; the count must be
    // rejected rather than committed to NVM.
    memset(N_storage_rw.metadatas, 0, sizeof(N_storage_rw.metadatas));
    const size_t entries = MAX_METADATA_COUNT + 1;
    for (size_t i = 0; i < entries; i++) {
        N_storage_rw.metadatas[i * 4] = 2;  // 4 bytes per record
        N_storage_rw.metadatas[i * 4 + 1] = META_NONE;
    }
    N_storage_rw.metadata_count = 0;

    TEST_ASSERT_EQUAL(ERR_NO_MORE_SPACE_AVAILABLE, compact_metadata());
    // Nothing was committed.
    TEST_ASSERT_EQUAL(0, N_storage_rw.metadata_count);
}

// --- bounded parser (V-008) -------------------------------------------------

static void test_compact_metadata_rejects_missing_terminator(void) {
    // A database whose records tile the whole array with no zero terminator used to walk the
    // parser past the end: the loop read METADATA_DATALEN(offset) before testing the bound.
    memset(N_storage_rw.metadatas, 0, sizeof(N_storage_rw.metadatas));
    for (size_t offset = 0; offset < sizeof(N_storage_rw.metadatas); offset += 4) {
        N_storage_rw.metadatas[offset] = 2;         // datalen: charset + 1 byte
        N_storage_rw.metadatas[offset + 1] = 0x00;  // META_NONE
    }

    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, compact_metadata());
}

static void test_compact_metadata_rejects_record_overrunning_the_end(void) {
    // A small, apparently valid record placed near the end of the array, declaring a payload
    // that runs past MAX_METADATAS. The parser has to walk there, so tile the space before it.
    memset(N_storage_rw.metadatas, 0, sizeof(N_storage_rw.metadatas));
    const size_t last = sizeof(N_storage_rw.metadatas) - 4;
    for (size_t offset = 0; offset < last; offset += 4) {
        N_storage_rw.metadatas[offset] = 2;  // datalen: charset + 1 byte -> 4 bytes total
        N_storage_rw.metadatas[offset + 1] = META_NONE;
    }
    // Only 4 bytes are left here, but this record claims MAX_METANAME + 2 == 23.
    N_storage_rw.metadatas[last] = MAX_METANAME;
    N_storage_rw.metadatas[last + 1] = META_NONE;

    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, compact_metadata());
}

static void test_compact_metadata_rejects_bad_kind(void) {
    add_password("alpha");
    N_storage_rw.metadatas[1] = 0x42;  // neither a known version nor META_ERASED

    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, compact_metadata());
}

static void test_compact_metadata_rejects_oversized_entry(void) {
    // Preserved behaviour: an entry longer than a full nickname is reported as too big, which
    // the restore path maps to SW_METADATAS_PARSING_ERROR.
    memset(N_storage_rw.metadatas, 0, sizeof(N_storage_rw.metadatas));
    N_storage_rw.metadatas[0] = MAX_METANAME + 1;
    N_storage_rw.metadatas[1] = 0x00;

    TEST_ASSERT_EQUAL(ERR_METADATA_ENTRY_TOO_BIG, compact_metadata());
}

static void test_get_metadata_terminates_on_corrupt_storage(void) {
    // get_metadata() used to loop forever on a database with no terminator.
    memset(N_storage_rw.metadatas, 0xFF, sizeof(N_storage_rw.metadatas));

    TEST_ASSERT_EQUAL(UINT32_MAX, get_metadata(0));
    TEST_ASSERT_EQUAL(MAX_METADATAS, find_free_metadata());
}

static void test_find_free_metadata_sentinel_is_unambiguous(void) {
    // dump_metadatas() tells a corrupt store from a real length by comparing against
    // MAX_METADATAS, so a well-formed store must never answer that value. It cannot: a
    // terminator has to sit inside the array to be read at all.
    TEST_ASSERT_EQUAL(0, find_free_metadata());  // empty store
    add_password("alpha");
    TEST_ASSERT_TRUE(find_free_metadata() < MAX_METADATAS);

    // Fill the store through the supported path and check the bound still holds when full.
    uint8_t name[MAX_METANAME];
    memset(name, 'A', sizeof(name));
    name[0] = 0x07;                   // charset
    name[1] = PASSWORD_DEFAULT_SIZE;  // password length
    while (write_metadata(name, sizeof(name), META_CURRENT_VERSION) == OK) {
        ;
    }
    TEST_ASSERT_TRUE(find_free_metadata() < MAX_METADATAS);
}

static void test_erase_metadata_rejects_out_of_range_offset(void) {
    add_password("alpha");

    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, erase_metadata(MAX_METADATAS));
    TEST_ASSERT_EQUAL(ERR_CORRUPTED_METADATA, erase_metadata(MAX_METADATAS - 1));
    // The live entry is untouched.
    assert_entry_is(raw_entry_offset(0), "alpha");
    TEST_ASSERT_EQUAL(1, N_storage_rw.metadata_count);
}

// --- password generation parameters ----------------------------------------

static void test_type_password_at_offset_version_0_entry(void) {
    // Charset 0 stands for all the sets, and version 0 has no length byte
    add_password("legacy");

    TEST_ASSERT_TRUE(type_password_at_offset(0));
    TEST_ASSERT_EQUAL(1, typed.calls);
    TEST_ASSERT_EQUAL_PTR(&N_storage_rw.metadatas[3], typed.data);
    TEST_ASSERT_EQUAL(6, typed.data_size);
    TEST_ASSERT_NULL(typed.out);
    TEST_ASSERT_EQUAL_HEX8(ALL_SETS, typed.sets);
    TEST_ASSERT_EQUAL(META_VERSION_0, typed.version);
    TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, typed.size);
}

static void test_show_password_at_offset_version_1_entry(void) {
    uint8_t out[PASSWORD_MAX_SIZE + 1];
    stub_charsets = 0x07;
    stub_password_length = 32;
    add_password("first");
    TEST_ASSERT_EQUAL(OK, create_new_password("mail", 4));
    const uint32_t offset = get_metadata(1);

    TEST_ASSERT_TRUE(show_password_at_offset(offset, out));
    TEST_ASSERT_EQUAL(1, typed.calls);
    TEST_ASSERT_EQUAL_PTR(&N_storage_rw.metadatas[offset + 4], typed.data);
    TEST_ASSERT_EQUAL_MEMORY("mail", typed.data, 4);
    TEST_ASSERT_EQUAL(4, typed.data_size);
    TEST_ASSERT_EQUAL_PTR(out, typed.out);
    TEST_ASSERT_EQUAL_HEX8(0x07, typed.sets);
    TEST_ASSERT_EQUAL(META_VERSION_1, typed.version);
    TEST_ASSERT_EQUAL(32, typed.size);
}

static void test_password_at_offset_reports_a_failure(void) {
    uint8_t out[PASSWORD_MAX_SIZE + 1];
    add_password("alpha");
    type_result = false;

    TEST_ASSERT_FALSE(type_password_at_offset(0));
    TEST_ASSERT_FALSE(show_password_at_offset(0, out));
    TEST_ASSERT_EQUAL(2, typed.calls);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nickname_exists_empty_db);
    RUN_TEST(test_nickname_exists_found);
    RUN_TEST(test_nickname_exists_not_found);
    RUN_TEST(test_nickname_exists_length_mismatch);
    RUN_TEST(test_nickname_exists_case_sensitive);
    RUN_TEST(test_nickname_exists_truncation);
    RUN_TEST(test_create_new_password_rejects_an_untruncatable_nickname);
    RUN_TEST(test_create_new_password_accepts_the_longest_storable_nickname);
    RUN_TEST(test_validate_metadata_image);
    RUN_TEST(test_validate_metadata_image_nicknames);
    RUN_TEST(test_charsets_are_weak);
    RUN_TEST(test_create_new_password_refuses_weak_charsets);
    RUN_TEST(test_validate_metadata_image_weak_charsets);
    RUN_TEST(test_password_length_fits_charsets);
    RUN_TEST(test_create_new_password_pin_needs_numbers);
    RUN_TEST(test_validate_metadata_image_pin_lengths);
    RUN_TEST(test_create_new_password_writes_the_current_version);
    RUN_TEST(test_validate_metadata_image_versions);
    RUN_TEST(test_version_0_entry_accessors);
    RUN_TEST(test_validate_metadata_image_lengths);
    RUN_TEST(test_validate_full_version_0_database);
    RUN_TEST(test_compaction_keeps_the_entry_version);
    RUN_TEST(test_commit_metadata_image);
    RUN_TEST(test_write_metadata_enforces_capacity);
    RUN_TEST(test_compact_metadata_erased_first_entry);
    RUN_TEST(test_compact_metadata_erased_middle_entry);
    RUN_TEST(test_compact_metadata_no_erased_entry);
    RUN_TEST(test_deleted_nickname_is_wiped);
    RUN_TEST(test_compaction_wipes_vacated_tail);
    RUN_TEST(test_compaction_removes_a_legacy_erased_nickname);
    RUN_TEST(test_restore_marker_lifecycle);
    RUN_TEST(test_abort_metadata_restore_clears_database);
    RUN_TEST(test_write_metadata_enforces_count_cap);
    RUN_TEST(test_compact_metadata_rejects_over_count_database);
    RUN_TEST(test_compact_metadata_rejects_missing_terminator);
    RUN_TEST(test_compact_metadata_rejects_record_overrunning_the_end);
    RUN_TEST(test_compact_metadata_rejects_bad_kind);
    RUN_TEST(test_compact_metadata_rejects_oversized_entry);
    RUN_TEST(test_get_metadata_terminates_on_corrupt_storage);
    RUN_TEST(test_find_free_metadata_sentinel_is_unambiguous);
    RUN_TEST(test_erase_metadata_rejects_out_of_range_offset);
    RUN_TEST(test_type_password_at_offset_version_0_entry);
    RUN_TEST(test_show_password_at_offset_version_1_entry);
    RUN_TEST(test_password_at_offset_reports_a_failure);
    return UNITY_END();
}
