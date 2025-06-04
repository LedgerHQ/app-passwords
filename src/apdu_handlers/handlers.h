#pragma once

#include <stdint.h>

#include "types.h"

#define TRANSFER_FLAG_OFFSET    0
#define TRANSFER_PAYLOAD_OFFSET 1

/* max payload size = 260 - sizeof(flag) - sizeof(status_word) */
#define MAX_PAYLOAD_SIZE (IO_APDU_BUFFER_SIZE - TRANSFER_PAYLOAD_OFFSET - 2)

#define MORE_DATA_INCOMING 0x00
#define LAST_CHUNK         0xFF

int dump_metadatas();
int get_app_config(uint8_t p1, uint8_t p2, const buf_t *input);
int load_metadatas(uint8_t p1, uint8_t p2, const buf_t *input);
