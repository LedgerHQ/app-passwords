#ifndef __METADATA_H__
#define __METADATA_H__

#include "stdint.h"

#define METADATA_PTR(offset)       (&N_storage.metadatas[offset])
#define METADATA_TOTAL_LEN(offset) (METADATA_DATALEN(offset) + 2)
#define METADATA_DATALEN(offset)   N_storage.metadatas[offset]  // charsets(1) + pwd seed(n)
#define METADATA_KIND(offset)      N_storage.metadatas[offset + 1]
#define METADATA_SETS(offset)      N_storage.metadatas[offset + 2]
/* even if the database is corrupted, this garantees we never overflow buffers of size
 * MAX_METANAME */
#define METADATA_NICKNAME_LEN(offset) ((METADATA_DATALEN(offset) - 1) % (MAX_METANAME + 1))
#define METADATA_NICKNAME(offset)     (&N_storage.metadatas[offset + 3])

#define META_NONE   0x00
#define META_ERASED 0xFF

typedef enum error_type_e {
    OK = 0,
    ERR_NO_MORE_SPACE_AVAILABLE,
    ERR_CORRUPTED_METADATA,
    ERR_NO_METADATA,
    ERR_METADATA_ENTRY_TOO_BIG
} error_type_t;

error_type_t write_metadata(uint8_t *data, uint8_t dataSize);
void reset_metadatas(void);
error_type_t erase_metadata(uint32_t offset);
uint32_t find_free_metadata(void);
uint32_t get_metadata(uint32_t nth);
error_type_t compact_metadata();

#endif