#include "error.h"

static const message_pair_t ERR_MESSAGES[5] = {
    {"", ""},                             // OK
    {"Write Error", "Database is full"},  // ERR_NO_MORE_SPACE_AVAILABLE
    {"Write Error",
     "Database should be repaired, please contact Ledger Support"},  // ERR_CORRUPTED_METADATA
    {"Erase Error", "Database already empty"},                       // ERR_NO_METADATA
    {"Write Error", "Entry is too big"}                              // ERR_METADATA_ENTRY_TOO_BIG
};

message_pair_t get_error(const error_type_t error) {
    return ERR_MESSAGES[error];
}
