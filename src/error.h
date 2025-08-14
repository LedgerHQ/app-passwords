#pragma once

#include "types.h"
#include "status_words.h"

#define SW_METADATAS_PARSING_ERROR 0x6F10

typedef enum error_type_e {
    OK = 0,
    ERR_NO_MORE_SPACE_AVAILABLE = 1,
    ERR_CORRUPTED_METADATA = 2,
    ERR_NO_METADATA = 3,
    ERR_METADATA_ENTRY_TOO_BIG = 4
} error_type_t;

message_pair_t get_error(const error_type_t error);
