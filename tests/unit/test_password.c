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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_nickname_exists_empty_db, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_found, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_not_found, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_length_mismatch, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_case_sensitive, setup, NULL),
        cmocka_unit_test_setup_teardown(test_nickname_exists_truncation, setup, NULL),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
