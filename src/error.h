#pragma once

#include "types.h"

#define SW_OK                              0x9000
#define SW_CONDITIONS_OF_USE_NOT_SATISFIED 0x6985
#define SW_WRONG_P1P2                      0x6A86
#define SW_WRONG_DATA_LENGTH               0x6A87
#define SW_INS_NOT_SUPPORTED               0x6D00
#define SW_CLA_NOT_SUPPORTED               0x6E00
#define SW_APPNAME_TOO_LONG                0xB000
#define SW_METADATAS_PARSING_ERROR         0x6F10

typedef enum error_type_e {
    OK = 0,
    ERR_NO_MORE_SPACE_AVAILABLE = 1,
    ERR_CORRUPTED_METADATA = 2,
    ERR_NO_METADATA = 3,
    ERR_METADATA_ENTRY_TOO_BIG = 4
} error_type_t;

message_pair_t get_error(const error_type_t error);
