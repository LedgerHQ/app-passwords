#pragma once

#include <io.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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
#if defined(SCREEN_SIZE_WALLET)
    uint8_t charset_options;
#endif
} internalStorage_t;

typedef enum {
    GET_APP_CONFIG = 0x03,
    DUMP_METADATAS = 0x04,
    LOAD_METADATAS = 0x05,
#ifdef TESTING
    RUN_TEST = 0x99
#endif
} cmd_e;

typedef struct app_state_s {
    struct {
        io_state_e state;
        size_t output_len;
    } io;
    cmd_e current_command;
    size_t bytes_transferred;
    bool user_approval;
} app_state_t;

typedef struct {
    uint8_t* bytes;
    size_t size;
} buf_t;

typedef struct message_pair_s {
    const char* first;
    const char* second;
} message_pair_t;
