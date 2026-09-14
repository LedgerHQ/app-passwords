#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_METADATAS 4096
#define MAX_METANAME  20
// Considering max metadata size (1+1+1+20) = 23, we can store at most 178 metadatas
#define MAX_METADATA_COUNT (MAX_METADATAS / (1 + 1 + 1 + MAX_METANAME))

typedef struct internalStorage_t {
#define STORAGE_MAGIC 0xDEAD1337
    uint32_t magic;
    bool press_enter_after_typing;
    uint8_t keyboard_layout;
    /**
     * A metadata in memory is represented by 1 byte of size (l), 1 byte of type (to disable it if
     * required), 1 byte to select char sets, l bytes of user seed
     */
    size_t metadata_count;
    uint8_t metadatas[MAX_METADATAS];
    uint8_t charset_options;
    /**
     * Set while a LOAD_METADATAS transfer is writing into `metadatas`, cleared once the image
     * has been validated. Found set at startup, it means a restore never completed and the
     * database is a half-written mix of the old and the new image.
     */
    uint8_t restore_in_progress;
} internalStorage_t;

/* `restore_in_progress` has to stay inside the padding that already followed
 * `charset_options`: the structure then keeps the size and the field offsets it had before,
 * and an app update reads the user's data where it left it. That holds exactly when the
 * structure ends one alignment unit past the offset of `charset_options`. */
_Static_assert(sizeof(internalStorage_t) ==
                   offsetof(internalStorage_t, charset_options) + _Alignof(internalStorage_t),
               "the fields after metadatas no longer fit in the existing tail padding: this "
               "moves persistent data and existing installs would read it at wrong offsets");

typedef enum {
    GET_APP_CONFIG = 0x03,
    DUMP_METADATAS = 0x04,
    LOAD_METADATAS = 0x05,
#ifdef TESTING
    RUN_TEST = 0x99
#endif
} cmd_e;

typedef struct app_state_s {
    size_t output_len;
    cmd_e current_command;
    size_t bytes_transferred;
    bool user_approval;
} app_state_t;

typedef struct {
    uint8_t *bytes;
    size_t size;
} buf_t;

typedef struct message_pair_s {
    const char *first;
    const char *second;
} message_pair_t;
